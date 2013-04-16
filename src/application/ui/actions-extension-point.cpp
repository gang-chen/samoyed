// Extension point - actions.
// Copyright (C) 2013 Gang Chen.

#include "actions-extension-point.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

#define ID "actions"

namespace Samoyed
{

ActionsExtensionPoint::ActionsExtensionPoint()
{
    ExtensionPoint::registerExtensionPoint(ID, this);
}

bool
ActionsExtensionPoint::addExtensionInternally(const char *pluginId,
                                              xmlDocPtr doc,
                                              xmlNodePtr node,
                                              std::list<std::string> &errors)
{
}

}
