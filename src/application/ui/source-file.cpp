// Opened source file.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "source-file.hpp"
#include "source-editor.hpp"
#include "project.hpp"
#include "application.hpp"
#include "resources/file-source-manager.hpp"
#include "resources/file-source.hpp"
#include "utilities/text-file-loader.hpp"
#include "utilities/property-tree.hpp"
#include <utility>
#include <list>
#include <string>
#include <glib/gi18n.h>

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
                       const char *mimeType,
                       const PropertyTree &options):
    TextFile(uri,
             mimeType,
             strcmp(options.name(), SOURCE_FILE_OPTIONS) == 0 ?
             options.child(TEXT_FILE_OPTIONS) : options)
{
    std::string uriEncoding;
    uriEncoding += uri;
    uriEncoding += '?';
    uriEncoding += encoding();
    m_source = Application::instance().fileSourceManager().
        reference(uriEncoding.c_str());
}

File *SourceFile::create(const char *uri,
                         const char *mimeType,
                         const PropertyTree &options)
{
    return new SourceFile(uri, mimeType, options);
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

SourceFile::~SourceFile()
{
    m_source->onFileClose(*this);
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

void SourceFile::onLoaded(FileLoader &loader)
{
    TextFile::onLoaded(loader);

    // Pass the loader's buffer to the source, which will own the buffer.
    TextFileLoader &ld = static_cast<TextFileLoader &>(loader);
    m_source->onFileLoaded(*this, ld.takeBuffer());
}

void SourceFile::onSaved(FileSaver &saver)
{
    m_source->onFileSaved(*this);
}

void SourceFile::onInserted(int line, int column,
                            const char *text, int length)
{
    m_source->onFileTextInserted(*this, line, column, text, length);
}

void SourceFile::onRemoved(int beginLine, int beginColumn,
                           int endLine, int endColumn)
{
    m_source->onFileTextRemoved(*this,
                                beginLine, beginColumn,
                                endLine, endColumn);
}

}
