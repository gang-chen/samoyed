// Compiler options collector.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "compiler-options-collector.hpp"
#include "project/project.hpp"
#include "project/project-db.hpp"
#include "utilities/utf8.hpp"
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>

namespace
{

const int BUFFER_SIZE = 10000;

}

namespace Samoyed
{

CompilerOptionsCollector::CompilerOptionsCollector(
    Scheduler &scheduler,
    unsigned int priority,
    Project &project,
    const char *inputFileName):
        Worker(scheduler, priority),
        m_project(project),
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
    }
    const char *valid;
    size += m_readPointer - m_readBuffer;
    if (!Utf8::validate(m_readBuffer, size, valid))
        return true;

    const char *parsePointer = m_readBuffer;
    if (!parse(parsePointer, valid))
        return true;

    size -= parsePointer - m_readBuffer;
    memmove(m_readBuffer, parsePointer, size);
    m_readPointer = m_readBuffer + size;
    return false;
}

bool CompilerOptionsCollector::parse(const char *&begin, const char *end)
{
    std::string uri;
    std::string compilerOpts;

    for (const char *cp = begin; begin < end; begin = cp)
    {
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
        const char *uriBegin = cp;
        while (*cp && cp < end)
            cp++;
        if (cp == end)
            return true;
        cp++;
        uri = uriBegin;

        // ""
        if (cp == end)
            return true;
        if (*cp)
            return false;
        cp++;

        // "CC ARGV:" or "CXX ARGV:"
        if (end - cp < 10)
            return true;
        if (strcmp(cp, "CC ARGV:") != 0)
            cp += 9;
        else if (strcmp(cp, "CXX ARGV:") != 0)
            cp += 10;
        else
            return false;

        // Scan the arguments and find the source file names.
        for (;;)
        {
            const char *argBegin = cp;
            while (*cp && cp < end)
                cp++;
            if (cp == end)
                return true;
            // ""
            if (cp == argBegin)
            {
                cp++;
                break;
            }
            cp++;
        }
    }

    return true;
}

}
