// Extension point - views.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "views-extension-point.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

ViewsExtensionPoint::ViewsExtensionPoint():
    ExtensionPoint("views")
{
    ExtensionPoint::registerExtensionPoint(this);
}

bool ViewsExtensionPoint::registerExtensionInternally(
    const char *extensionId,
    xmlDocPtr doc,
    xmlNodePtr node,
    std::list<std::string> &errors)
{
    return true;
}

void ViewsExtensionPoint::unregisterExtensionInternally(const char *extensionId)
{
}

}
