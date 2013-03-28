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
#include <list>
#include <map>
#include <boost/any.hpp>
#include <glib/gi18n-lib.h>

#define ENCODING "encoding"

namespace
{

const char *mimeTypes[] =
{ "text/x-csrc", "text/x-chdr", "text/x-c++src", "text/x-c++hdr", NULL };

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

bool SourceFile::isSupportedType(const char *type)
{
    for (const char **mimeType = mimeTypes; *mimeType; ++mimeType)
    {
        char *cType = g_content_type_from_mime_type(*mimeType);
        bool supported = g_content_type_is_a(type, cType);
        g_free(cType);
        if (supported)
            return true;
    }
    return false;
}

void SourceFile::registerType()
{
    for (const char **mimeType = mimeTypes; *mimeType; ++mimeType)
    {
        char *type = g_content_type_from_mime_type(*mimeType);
        File::registerType(type, create);
        g_free(type);
    }
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
