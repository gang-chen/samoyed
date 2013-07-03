// Extension: view.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_VIEW_EXTENSION_HPP
#define SMYD_VIEW_EXTENSION_HPP

#include "../utilities/extension.hpp"

namespace Samoyed
{

class View;

class ViewExtension: public Extension
{
public:
    ViewExtension(const char *id, Plugin &plugin):
        Extension(id, plugin)
    {}

    virtual View *createView() = 0;
};

}

#endif
