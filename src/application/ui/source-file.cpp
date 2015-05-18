// Opened source file.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "source-file.hpp"
#include "source-editor.hpp"
#include "project.hpp"
#include "application.hpp"
#include "parsers/foreground-file-parser.hpp"
#include "utilities/text-file-loader.hpp"
#include "utilities/property-tree.hpp"
#include <utility>
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <clang-c/Index.h>

#define TEXT_FILE_OPTIONS "text-file-options"
#define SOURCE_FILE_OPTIONS "source-file-options"

namespace
{

struct MimeType
{
    const char *mimeType;
    const char *description;
};

const MimeType mimeTypes[] =
{
    { "text/x-csrc",    N_("C Source Files") },
    { "text/x-chdr",    N_("C Header Files") },
    { "text/x-c++src",  N_("C++ Source Files") },
    { "text/x-c++hdr",  N_("C++ Header Files") },
    { NULL,             NULL }
};

struct MimeTypeSet
{
    const char *mimeTypeSet;
    const char *masterMimeType;
    const char *description;
};

const MimeTypeSet mimeTypeSets[] =
{
    {
        "text/x-csrc text/x-chdr text/x-c++src text/x-c++hdr",
        "text/x-csrc",
        N_("C/C++ Source and Header Files")
    },
    {
        NULL,
        NULL,
        NULL
    }
};

}

namespace Samoyed
{

PropertyTree *SourceFile::OptionsSetter::options() const
{
    PropertyTree *options = new PropertyTree(SOURCE_FILE_OPTIONS);
    options->addChild(*TextFile::OptionsSetter::options());
    return options;
}

File::OptionsSetter *SourceFile::createOptionsSetter()
{
    return new SourceFile::OptionsSetter;
}

PropertyTree SourceFile::s_defaultOptions(SOURCE_FILE_OPTIONS);

const PropertyTree &SourceFile::defaultOptions()
{
    if (s_defaultOptions.empty())
        s_defaultOptions.
            addChild(*(new PropertyTree(TextFile::defaultOptions())));
    return s_defaultOptions;
}

// It is possible that options for text files are given.
bool SourceFile::optionsEqual(const PropertyTree &options1,
                              const PropertyTree &options2)
{
    return TextFile::optionsEqual(
            strcmp(options1.name(), SOURCE_FILE_OPTIONS) == 0 ?
            options1.child(TEXT_FILE_OPTIONS) : options1,
            strcmp(options2.name(), SOURCE_FILE_OPTIONS) == 0 ?
            options2.child(TEXT_FILE_OPTIONS) : options2);
}

// It is possible that options for text files are given.
void SourceFile::describeOptions(const PropertyTree &options,
                                 std::string &desc)
{
    TextFile::describeOptions(strcmp(options.name(), SOURCE_FILE_OPTIONS) == 0 ?
                              options.child(TEXT_FILE_OPTIONS) : options,
                              desc);
}

// It is possible that options for text files are given.
SourceFile::SourceFile(const char *uri,
                       int type,
                       const char *mimeType,
                       const PropertyTree &options):
    TextFile(uri,
             type,
             mimeType,
             strcmp(options.name(), SOURCE_FILE_OPTIONS) == 0 ?
             options.child(TEXT_FILE_OPTIONS) : options),
    m_needReparse(false)
{
}

SourceFile::~SourceFile()
{
}

File *SourceFile::create(const char *uri,
                         const char *mimeType,
                         const PropertyTree &options)
{
    return new SourceFile(uri, TYPE, mimeType, options);
}

bool SourceFile::isSupportedType(const char *mimeType)
{
    bool supported = false;
    char *type = g_content_type_from_mime_type(mimeType);
    for (const MimeType *mimeType2 = mimeTypes;
         mimeType2->mimeType;
         ++mimeType2)
    {
        char *type2 = g_content_type_from_mime_type(mimeType2->mimeType);
        supported = g_content_type_is_a(type, type2);
        g_free(type2);
        if (supported)
            break;
    }
    g_free(type);
    return supported;
}

void SourceFile::registerType()
{
    for (const MimeType *mimeType = mimeTypes; mimeType->mimeType; ++mimeType)
        File::registerType(mimeType->mimeType,
                           create,
                           createOptionsSetter,
                           defaultOptions,
                           optionsEqual,
                           describeOptions,
                           gettext(mimeType->description));
    for (const MimeTypeSet *mimeTypeSet = mimeTypeSets;
         mimeTypeSet->mimeTypeSet;
         ++mimeTypeSet)
        File::registerTypeSet(mimeTypeSet->mimeTypeSet,
                              mimeTypeSet->masterMimeType,
                              gettext(mimeTypeSet->description));
}

PropertyTree *SourceFile::options() const
{
    PropertyTree *options = new PropertyTree(SOURCE_FILE_OPTIONS);
    options->addChild(*TextFile::options());
    return options;
}

Editor *SourceFile::createEditorInternally(Project *project)
{
    return SourceEditor::create(*this, project);
}

void SourceFile::onLoaded(const boost::shared_ptr<FileLoader> &loader)
{
    TextFile::onLoaded(loader);
    if (m_tu)
    {
        boost::shared_ptr<CXTranslationUnitImpl> tu;
        tu.swap(m_tu);
        Application::instance().foregroundFileParser().reparse(uri(), tu);
    }
    else
        Application::instance().foregroundFileParser().parse(uri());
}

void SourceFile::onChanged(const File::Change &change, bool loading)
{
    TextFile::onChanged(change, loading);
    if (loading)
        return;
    if (m_tu)
    {
        boost::shared_ptr<CXTranslationUnitImpl> tu;
        tu.swap(m_tu);
        Application::instance().foregroundFileParser().reparse(uri(), tu);
    }
    else
        m_needReparse = true;
}

void SourceFile::onParseDone(boost::shared_ptr<CXTranslationUnitImpl> tu,
                             int error)
{
    if (error)
    {
        // Do not disturb the user.  Log this Clang error.
        switch (error)
        {
        case CXError_Failure:
            g_warning(_("Clang failed to parse file \"%s\"."), uri());
            break;
        case CXError_Crashed:
            g_warning(_("Clang crashed when parsing file \"%s\"."), uri());
            break;
        case CXError_InvalidArguments:
            g_warning(_("Clang failed to parse file \"%s\" due to invalid "
                        "arguments."),
                      uri());
            break;
        case CXError_ASTReadError:
            g_warning(_("Clang failed to parse file \"%s\" due to an AST "
                        "deserialization error."),
                      uri());
            break;
        }
    }
    if (m_needReparse)
    {
        if (tu)
            Application::instance().foregroundFileParser().reparse(uri(), tu);
        else
            Application::instance().foregroundFileParser().parse(uri());
        m_needReparse = false;
    }
    else
    {
        m_tu.swap(tu);
        highlightTokens();
    }
}

void
SourceFile::onCodeCompletionDone(boost::shared_ptr<CXTranslationUnitImpl> tu,
                                 CXCodeCompleteResults *results)
{
    if (!results)
    {
        // Do not disturb the user.  Log this Clang error.
        g_warning(_("Clang failed to complete code in file \"%s\"."), uri());
    }
}

void SourceFile::highlightTokens()
{
    for (SourceEditor *editor = static_cast<SourceEditor *>(editors());
         editor;
         editor = static_cast<SourceEditor *>(editor->nextInFile()))
        editor->cleanTokens(0, 0, -1, -1);

    if (!m_tu)
        return;

    CXTranslationUnit tu = m_tu.get();
    char *fileName = g_filename_from_uri(uri(), NULL, NULL);
    CXFile file = clang_getFile(tu, fileName);
    g_free(fileName);
    CXSourceLocation begin = clang_getLocationForOffset(tu, file, 0);
    CXSourceLocation end =
        clang_getLocationForOffset(tu, file, characterCount());
    CXSourceRange range = clang_getRange(begin, end);
    CXToken *tokens;
    unsigned int numTokens;
    clang_tokenize(tu, range, &tokens, &numTokens);

    for (unsigned int i = 0; i < numTokens; ++i)
    {
        CXSourceRange range = clang_getTokenExtent(tu, tokens[i]);
        CXSourceLocation begin = clang_getRangeStart(range);
        unsigned int beginLine, beginColumn;
        clang_getFileLocation(begin, NULL, &beginLine, &beginColumn, NULL);
        CXSourceLocation end = clang_getRangeEnd(range);
        unsigned int endLine, endColumn;
        clang_getFileLocation(end, NULL, &endLine, &endColumn, NULL);

        for (SourceEditor *editor = static_cast<SourceEditor *>(editors());
             editor;
             editor = static_cast<SourceEditor *>(editor->nextInFile()))
        {
            editor->highlightToken(beginLine - 1, beginColumn - 1,
                                   endLine - 1, endColumn - 1,
                                   clang_getTokenKind(tokens[i]));
        }
    }
}

}
