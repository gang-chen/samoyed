// Extension: view.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_VIEW_EXTENSION_HPP
#define SMYD_VIEW_EXTENSION_HPP

#include "plugin/extension.hpp"
#include "view.hpp"

namespace Samoyed
{

class ViewExtension: public Extension
{
public:
    ViewExtension(const char *id, Plugin &plugin):
        Extension(id, plugin)
    {}

    virtual View *createView(const char *viewId, const char *viewTitle) = 0;
    virtual View *restoreView(View::XmlElement &xmlElement) = 0;

    virtual void registerXmlElementReader() = 0;
    virtual void unregisterXmlElementReader() = 0;
};

}

#endif
