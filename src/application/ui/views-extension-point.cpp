// Extension point - views.
// Copyright (C) 2013 Gang Chen.

#include "views-extension-point.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

#define ID "views"

namespace Samoyed
{

ViewsExtensionPoint::ViewsExtensionPoint()
{
    ExtensionPoint::registerExtensionPoint(ID, this);
}

bool ViewsExtensionPoint::addExtensionInternally(const char *pluginId,
                                                 xmlDocPtr doc,
                                                 xmlNodePtr node,
                                                 std::list<std::string> &errors)
{
}

}
