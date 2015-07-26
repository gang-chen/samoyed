// Extension: file observer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FILE_OBSERVER_EXTENSION_HPP
#define SMYD_FILE_OBSERVER_EXTENSION_HPP

#include "plugin/extension.hpp"

namespace Samoyed
{

class File;
class FileObserver;

class FileObserverExtension: public Extension
{
public:
    FileObserverExtension(const char *id, Plugin &plugin):
        Extension(id, plugin)
    {}

    virtual FileObserver *activateObserver(File &file) = 0;
};

}

#endif
