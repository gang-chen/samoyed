// File type registry.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_FILE_TYPE_REGISTRY_HPP
#define SMYD_FILE_TYPE_REGISTRY_HPP

#include <map>
#include <string>
#include <boost/function.hpp>

namespace Samoyed
{

class File;

class FileTypeRegistry
{
public:
    typedef boost::function<File *(const char *)> FileFactory;

    void registerFileFactory(const char *mime, const FileFactory &factory);

    const FileFactory &getParserFactory(const char *uri) const;

private:
    typedef std::map<std::string, FileFactory> FileFactoryTable;

    FileFactoryTable m_fileFactories;
};

}

#endif
