// File type registry.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_FILE_TYPE_REGISTRY_HPP
#define SMYD_FILE_TYPE_REGISTRY_HPP

#include <map>
#include <string>
#include <boost/utility.hpp>
#include <boost/function.hpp>

namespace Samoyed
{

class File;

class FileTypeRegistry: public boost::noncopyable
{
public:
    typedef boost::function<File *(const char *uri)> FileFactory;

    static char *getFileType(const char *uri);

    void registerFileFactory(const char *mimeType, const FileFactory &factory);

    const FileFactory *getFileFactory(const char *mimeType) const;

private:
    typedef std::map<std::string, FileFactory> FileFactoryTable;

    FileFactoryTable m_fileFactoryTable;
};

}

#endif
