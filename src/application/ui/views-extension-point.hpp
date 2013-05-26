// Extension point - views.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_VIEWS_EXTENSION_POINT_HPP
#define SMYD_VIEWS_EXTENSION_POINT_HPP

#include <list>
#include <string>
#include <libxml/tree.h>
#include "../utilities/extension-point.hpp"

namespace Samoyed
{

class ViewsExtensionPoint: public ExtensionPoint
{
public:
    ViewsExtensionPoint();

    virtual bool registerExtension(const char *pluginId,
                                   const char *extensionId,
                                   xmlDocPtr doc,
                                   xmlNodePtr node,
                                   std::list<std::string> &errors);

    virtual void unregisterExtension(const char *extensionId);

    virtual bool activateExtension(const char *extensionId);
};

}

#endif
