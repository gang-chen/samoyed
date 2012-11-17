// File type registry.
// Copyright (C) 2012 Gang Chen.

#include "file-type-registry.hpp"
#include <map>
#include <string>
#include <utility>
#include <boost/function.hpp>
#include <glib.h>

namespace Samoyed
{

static char *FileTypeRegistry::getFileType(const char *uri)
{
    char *fileName = g_filename_from_uri(uri, NULL, NULL);
    if (!fileName)
        return NULL;
    char *type = g_content_type_guess(fileName, NULL, 0, NULL);
    g_free(fileName);
    if (!type)
        return NULL;
    char *mimeType = g_content_type_get_mime_type(type);
    g_free(type);
    return mimeType;
}

FileTypeRegistry::~FileTypeRegistry()
{
    if (FileFactoryTable::const_iterator it = m_fileFactoryTable.begin();
        it != m_fileFactoryTable.end();
        ++it)
        delete it->second;
}

void FileTypeRegistry::registerFileFactory(const char *mimeType,
                                           const FileFactory &factory)
{
    m_fileFactoryTable.insert(std::make_pair(std::string(mimeType),
                                             new FileFactory(factory)));
}

const FileFactory *FileTypeRegistry::getFileFactory(const char *mimeType) const
{
    FileFactoryTable::const_iterator it =
        m_fileFactoryTable.find(std::string(mimeType));
    if (it == m_fileFactoryTable.end())
        return NULL;
    return it->second;
}

}
