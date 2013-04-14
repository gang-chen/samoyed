// Extension point - action.
// Copyright (C) 2013 Gang Chen.

#include "action-extension-point.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

#define ID "action"

namespace Samoyed
{

ActionExtensionPoint::ActionExtensionPoint()
{
    ExtensionPoint::registerExtensionPoint(ID, this);
}

bool
ActionExtensionPoint::addExtensionInternally(const char *pluginId,
                                             xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &strings)
{
}

}
