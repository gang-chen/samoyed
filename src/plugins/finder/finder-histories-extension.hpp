// Extension: finder histories.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_FIND_FINDER_HISTORIES_EXTENSION_HPP
#define SMYD_FIND_FINDER_HISTORIES_EXTENSION_HPP

#include "session/histories-extension.hpp"

namespace Samoyed
{

namespace Finder
{

class FinderHistoriesExtension: public HistoriesExtension
{
public:
    FinderHistoriesExtension(const char *id, Plugin &plugin):
        HistoriesExtension(id, plugin)
    {}

    virtual void installHistories();

    virtual void uninstallHistories();
};

}

}

#endif
