// Extension point - actions.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "actions-extension-point.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

ActionsExtensionPoint::ActionsExtensionPoint():
    ExtensionPoint("actions")
{
    ExtensionPoint::registerExtensionPoint(this);
}

bool ActionsExtensionPoint::registerExtensionInternally(
    const char *extensionId,
    xmlDocPtr doc,
    xmlNodePtr node,
    std::list<std::string> &errors)
{
    return true;
}

void
ActionsExtensionPoint::unregisterExtensionInternally(const char *extensionId)
{
}

}
