// File type registry.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_FILE_TYPE_REGISTRY_HPP
#define SMYD_FILE_TYPE_REGISTRY_HPP

#include <utility>
#include <map>
#include <string>
#include <boost/utility>
#include <boost/function.hpp>
#include <gio/gio.h>
#include <glib.h>

namespace Samoyed
{

class File;

class FileTypeRegistry: public boost::noncopyable
{
public:
    typedef boost::function<File *(const char *)> FileFactory;

    static char *getFileType(const char *uri)
    {
        // 'g_content_type_guess()' can handle the URI as a file name well.
        char *type = g_content_type_guess(uri, NULL, 0, NULL);
        if (!type)
            return NULL;
        char *mimeType = g_content_type_get_mime_type(type);
        g_free(type);
        return mimeType;
    }

    void registerFileFactory(const char *mimeType, const FileFactory &factory)
    {
        m_fileFactoryTable.insert(std::make_pair(std::string(mimeType),
                                                 factory));
    }

    const FileFactory *getFileFactory(const char *mimeType) const
    {
        FileFactoryTable::const_iterator it =
            m_fileFactoryTable.find(std::string(mimeType));
        if (it != m_fileFactoryTable.end())
            return NULL;
        return &it->second;
    }

private:
    typedef std::map<std::string, FileFactory> FileFactoryTable;

    FileFactoryTable m_fileFactoryTable;
};

}

#endif
