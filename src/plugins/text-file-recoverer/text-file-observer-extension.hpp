// File observer extension: text file observer for saving text edits.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_TXRC_TEXT_FILE_OBSERVER_EXTENSION_HPP
#define SMYD_TXRC_TEXT_FILE_OBSERVER_EXTENSION_HPP

#include "ui/file-observer-extension.hpp"

namespace Samoyed
{

namespace TextFileRecoverer
{

class TextFileObserverExtension: public FileObserverExtension
{
public:
    TextFileObserverExtension(const char *id, Plugin &plugin):
        FileObserverExtension(id, plugin)
    {}

    virtual FileObserver *activateObserver(File &file);
};

}

}

#endif
