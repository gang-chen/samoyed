// Extension: histories.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_HISTORIES_EXTENSION_HPP
#define SMYD_HISTORIES_EXTENSION_HPP

#include "utilities/extension.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

class HistoriesExtension: public Extension
{
public:
    HistoriesExtension(const char *id, Plugin &plugin):
        Extension(id, plugin)
    {}

    virtual void installHistories() = 0;

    virtual void uninstallHistories() = 0;
};

}

#endif
