// Extension: view.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_VIEW_EXTENSION_HPP
#define SMYD_VIEW_EXTENSION_HPP

#include "../utilities/extension.hpp"
#include "widget.hpp"

namespace Samoyed
{

class ViewExtension: public Extension
{
public:
    ViewExtension(const char *id,
                  Plugin &plugin,
                  ExtensionPoint &extensionPoint):
        Extension(id, plugin, extensionPoint)
    {}

    virtual Widget *createView() = 0;
};

}

#endif
