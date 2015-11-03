// Compiler options collector.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "compiler-options-collector.hpp"
#include "project/project.hpp"
#include "project/project-db.hpp"
#include "utilities/miscellaneous.hpp"
#include <string.h>
#include <list>
#include <string>
#include <set>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

namespace
{

const int BUFFER_SIZE = 10000;

const char *SOURCE_FILE_NAME_SUFFIXES[] =
{
    ".c",
    ".cc",
    ".cp",
    ".cxx",
    ".cpp",
    ".CPP",
    ".c++",
    ".C",
    ".h",
    ".hh",
    ".hpp",
    ".H"
#ifdef OS_WINDOWS
    // Mixed case letters are not allowed.
    ,
    ".CC",
    ".CP",
    ".CXX",
    ".C++",
    ".HH",
    ".HPP"
#endif
};

const char *OPTIONS_NEEDING_ARGUMENTS[] =
{
    "-D",
    "-I",
    "-MF",
    "-MQ",
    "-MT",
    "-T",
    "-U",
    "-V",
    "-Xassembler",
    "-Xlinker",
    "-Xpreprocessor",
    "-aux-info",
    "-b",
    "-idirafter",
    "-imacros",
    "-imultilib",
    "-include",
    "-iprefix",
    "-iquote",
    "-isysroot",
    "-isystem",
    "-iwithprefix",
    "-iwithprefixbefore",
    "-l",
    "-o",
    "-u",
    "-x",
    NULL
};

const char *OPTIONS_FILTERED_OUT[] =
{
    "-E",
    "-M",
    "-MD",
    "-MF",
    "-MG",
    "-MM",
    "-MMD",
    "-MP",
    "-MQ",
    "-MT",
    "-S",
    "-aux-info",
    "-c",
    "-o",
    NULL
};

const char *DIR_OPTIONS[] =
{
    "-I",
    "-L",
    NULL
};

std::set<Samoyed::ComparablePointer<const char> > sourceFileNameSuffixes;

std::set<Samoyed::ComparablePointer<const char> > optionsNeedingArguments;

std::set<Samoyed::ComparablePointer<const char> > optionsFilteredOut;

}

namespace Samoyed
{

CompilerOptionsCollector::CompilerOptionsCollector(
    Scheduler &scheduler,
    unsigned int priority,
    Project &project,
    const char *inputFileName):
        Worker(scheduler, priority),
        m_projectDb(project.db()),
        m_inputFileName(inputFileName),
        m_stream(NULL),
        m_readBuffer(NULL)
{
    char *desc =
        g_strdup_printf(_("Collecting compiler options from file \"%s\" for "
                          "project \"%s\"."),
                        inputFileName, project.uri());
    setDescription(desc);
    g_free(desc);

    if (sourceFileNameSuffixes.empty())
    {
        for (const char **suffix = SOURCE_FILE_NAME_SUFFIXES; *suffix; suffix++)
            sourceFileNameSuffixes.insert(*suffix);
    }
    if (optionsNeedingArguments.empty())
    {
        for (const char **opt = OPTIONS_NEEDING_ARGUMENTS; *opt; opt++)
            optionsNeedingArguments.insert(*opt);
    }
    if (optionsFilteredOut.empty())
    {
        for (const char **opt = OPTIONS_FILTERED_OUT; *opt; opt++)
            optionsFilteredOut.insert(*opt);
    }
}

CompilerOptionsCollector::~CompilerOptionsCollector()
{
    if (m_stream)
        g_object_unref(m_stream);
    delete[] m_readBuffer;
}

bool CompilerOptionsCollector::step()
{
    GError *error = NULL;

    if (!m_stream)
    {
        GFile *file = g_file_new_for_path(m_inputFileName.c_str());
        GFileInputStream *fileStream = g_file_read(file, NULL, &error);
        g_object_unref(file);
        if (!fileStream)
        {
            g_error_free(error);
            return true;
        }

#ifdef OS_WINDOWS
        const char *encoding = "UTF-16";
#else
        const char *encoding;
        if (g_get_charset(&encoding))
            m_stream = G_INPUT_STREAM(fileStream);
        else
        {
#endif
        GCharsetConverter *encodingConverter =
            g_charset_converter_new("UTF-8", encoding, &error);
        if (!encodingConverter)
        {
            g_object_unref(fileStream);
            return true;
        }
        m_stream =
            g_converter_input_stream_new(G_INPUT_STREAM(fileStream),
                                         G_CONVERTER(encodingConverter));
        g_object_unref(fileStream);
        g_object_unref(encodingConverter);
#ifndef OS_WINDOWS
        }
#endif

        m_readBuffer = new char[BUFFER_SIZE];
        m_readPointer = m_readBuffer;
    }

    int size = g_input_stream_read(m_stream,
                                   m_readPointer,
                                   m_readBuffer + BUFFER_SIZE - m_readPointer,
                                   NULL,
                                   &error);
    if (size == -1)
    {
        g_error_free(error);
        return true;
    }
    if (size == 0)
    {
        if (m_readPointer != m_readBuffer)
            return true;
        if (!g_input_stream_close(m_stream, NULL, &error))
        {
            g_error_free(error);
            return true;
        }
        return true;
    }
    m_readPointer += size;

    const char *parsePointer = m_readBuffer;
    if (!parse(parsePointer, m_readPointer))
        return true;

    size = m_readPointer - parsePointer;
    memmove(m_readBuffer, parsePointer, size);
    m_readPointer = m_readBuffer + size;
    return false;
}

bool CompilerOptionsCollector::parse(const char *&begin, const char *end)
{
    for (const char *cp = begin; begin < end; begin = cp)
    {
        const char *cwd;
        std::list<const char *> fileNames;
        std::string compilerOpts;

        // "EXE:"
        if (end - cp < 5)
            return true;
        if (strcmp(begin, "EXE:") != 0)
            return false;
        cp += 5;

        // "..."
        while (cp < end && *cp)
            cp++;
        if (cp == end)
            return true;
        cp++;

        // ""
        if (cp == end)
            return true;
        if (*cp)
            return false;
        cp++;

        // "CWD:"
        if (end - cp < 5)
            return true;
        if (strcmp(cp, "CWD:") != 0)
            return false;
        cp += 5;

        // "..."
        cwd = cp;
        while (*cp && cp < end)
            cp++;
        if (cp == end)
            return true;
        cp++;

        // ""
        if (cp == end)
            return true;
        if (*cp)
            return false;
        cp++;

        // "CC ARGV:" or "CXX ARGV:"
        if (end - cp < 10)
            return true;
        if (strcmp(cp, "CC ARGV:") == 0)
            cp += 9;
        else if (strcmp(cp, "CXX ARGV:") == 0)
            cp += 10;
        else
            return false;

        // Scan the arguments and find the source file names.
        bool first = true;
        for (;;)
        {
            const char *arg = cp;
            while (*cp && cp < end)
                cp++;
            if (cp == end)
                return true;
            // The terminating "".
            if (cp == arg)
            {
                cp++;
                break;
            }
            cp++;

            if (first)
            {
                first = false;
                continue;
            }

            if (arg[0] != '-')
            {
                // A source file name is found.
                fileNames.push_back(arg);
            }
            else if (optionsNeedingArguments.find(arg) ==
                     optionsNeedingArguments.end())
            {
                // A compiler option is found.
                if (optionsFilteredOut.find(arg) == optionsFilteredOut.end())
                {
                    const char **opt;
                    for (opt = DIR_OPTIONS; *opt; opt++)
                    {
                        int l = strlen(*opt);
                        if (strncmp(*opt, arg, l) == 0)
                        {
                            compilerOpts.append(arg, l);
                            arg += l;
                            if (g_path_is_absolute(arg))
                                compilerOpts.append(arg, cp - arg);
                            else
                            {
                                char *fn = g_build_filename(cwd, arg, NULL);
                                compilerOpts.append(fn, strlen(fn) + 1);
                                g_free(fn);
                            }
                            break;
                        }
                    }
                    if (!*opt)
                        compilerOpts.append(arg, cp - arg);
                }
            }
            else
            {
                // This compiler option requires an argument.
                const char *arg2 = cp;
                while (*cp && cp < end)
                    cp++;
                if (cp == end)
                    return true;
                // The terminating "".
                if (cp == arg2)
                    return false;
                cp++;

                if (optionsFilteredOut.find(arg) == optionsFilteredOut.end())
                {
                    const char **opt;
                    for (opt = DIR_OPTIONS; *opt; opt++)
                    {
                        if (strcmp(*opt, arg) == 0)
                        {
                            compilerOpts.append(arg, arg2 - arg);
                            if (g_path_is_absolute(arg2))
                                compilerOpts.append(arg2, cp - arg2);
                            else
                            {
                                char *fn = g_build_filename(cwd, arg2, NULL);
                                compilerOpts.append(fn, strlen(fn) + 1);
                                g_free(fn);
                            }
                            break;
                        }
                    }
                    if (!*opt)
                        compilerOpts.append(arg, cp - arg);
                }
            }
        }

        // ""
        if (cp == end)
            return true;
        if (*cp)
            return false;
        cp++;

        for (std::list<const char *>::const_iterator it = fileNames.begin();
             it != fileNames.end();
             ++it)
        {
            char *buf = NULL;
            const char *fn;
            if (g_path_is_absolute(*it))
                fn = *it;
            else
            {
                buf = g_build_filename(cwd, *it, NULL);
                fn = buf;
            }

            // Check to see if this is a source file.
            char *ext = strrchr(fn, '.');
            if (!ext ||
                sourceFileNameSuffixes.find(ext) ==
                sourceFileNameSuffixes.end())
            {
                g_free(buf);
                continue;
            }

            // Write the compiler options.
            char *uri = g_filename_to_uri(fn, NULL, NULL);
            m_projectDb.writeCompilerOptions(uri,
                                             compilerOpts.c_str(),
                                             compilerOpts.length());
            g_free(uri);
        }
    }

    return true;
}

}
