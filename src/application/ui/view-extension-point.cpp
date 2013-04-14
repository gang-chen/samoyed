// Extension point - view.
// Copyright (C) 2013 Gang Chen.

#include "view-extension-point.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

#define ID "view"

namespace Samoyed
{

ViewExtensionPoint::ViewExtensionPoint()
{
    ExtensionPoint::registerExtensionPoint(ID, this);
}

bool ViewExtensionPoint::addExtensionInternally(const char *pluginId,
                                                xmlDocPtr doc,
                                                xmlNodePtr node,
                                                std::list<std::string> &strings)
{
}

}
