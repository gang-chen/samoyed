// File source manager.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FILE_SOURCE_MANAGER_HPP
#define SMYD_FILE_SOURCE_MANAGER_HPP

#include "../utilities/manager.hpp"
#include "file-source.hpp"

namespace Samoyed
{

class FileSourceManager: public Manager<FileSource>
{
private:
    static const int CACHE_SIZE = 20;

public:
    FileSourceManager(): Manager<FileSource>(CACHE_SIZE) {}
};

}

#endif
