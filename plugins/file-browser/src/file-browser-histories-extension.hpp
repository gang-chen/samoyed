// Extension: file browser histories.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_FLBR_FILE_BROWSER_HISTORIES_EXTENSION_HPP
#define SMYD_FLBR_FILE_BROWSER_HISTORIES_EXTENSION_HPP

#include "session/histories-extension.hpp"

namespace Samoyed
{

namespace FileBrowser
{

class FileBrowserHistoriesExtension: public HistoriesExtension
{
public:
    FileBrowserHistoriesExtension(const char *id, Plugin &plugin):
        HistoriesExtension(id, plugin)
    {}

    virtual void installHistories();

    virtual void uninstallHistories();
};

}

}

#endif
