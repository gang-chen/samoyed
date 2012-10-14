// Manager of opened files.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_FILE_MANAGER_HPP
#define SMYD_FILE_MANAGER_HPP

#include "utilities/pointer-comparator.hpp"
#include <map>
#include <string>
#include <boost/signals2/signal.hpp>

namespace Samoyed
{

class File;

class FileManager
{
public:
    typedef boost::signals2::signal<void (const File &file)> FileOpened;

    File *get(const char *uri);

    File &open(const char *uri);

    void close(File &file);

private:
    typedef std::map<std::string *, File *, PointerComparator<std::string> >
    	FileStore;

    FileStore m_files;

    FileOpened m_fileOpened;
};

}

#endif
