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

protected:
    virtual bool registerExtensionInternally(const char *pluginId,
                                             const char *extensionId,
                                             xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &errors);

    virtual bool unregisterExtensionInternally(const char *pluginId,
                                               const char *extensionId);
};

}

#endif
