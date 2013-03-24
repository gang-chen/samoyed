// Opened source file.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "source-file.hpp"
#include "source-editor.hpp"
#include "project.hpp"
#include "../application.hpp"
#include "../resources/file-source-manager.hpp"
#include "../resources/file-source.hpp"
#include "../utilities/text-file-loader.hpp"
#include <utility>
#include <map>
#include <boost/any.hpp>
#include <glib/gi18n-lib.h>

#define ENCODING "encoding"

namespace
{

struct TypeEntry
{
    const char *mimeType;
    const char *description;
};

const TypeEntry typeEntries[] =
{
    { "text/x-csrc", N_("C source") },
    { "text/x-chdr", N_("C header") },
    { "text/x-c++src", N_("C++ source") },
    { "text/x-c++hdr", N_("C++ header") },
    { NULL, NULL }
};

}

namespace Samoyed
{

File *SourceFile::create(const char *uri, Project *project,
                         const std::map<std::string, boost::any> &options)
{
    std::string encoding("UTF-8");
    std::map<std::string, boost::any>::const_iterator it =
        options.find(ENCODING);
    if (it != options.end())
        encoding = boost::any_cast<std::string>(it->second);
    else
    {
    }

    return new SourceFile(uri, encoding.c_str());
}

void SourceFile::registerType()
{
    for (const TypeEntry *entry = typeEntries; entry->mimeType; ++entry)
        File::registerType(entry->mimeType, entry->description, create);
}

SourceFile::SourceFile(const char *uri, const char *encoding):
    TextFile(uri, encoding)
{
    m_source = Application::instance().fileSourceManager().reference(uri);
}

SourceFile::~SourceFile()
{
    m_source->onFileClose(*this);
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
