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

bool
ActionsExtensionPoint::addExtensionInternally(const char *pluginId,
                                              const char *extensionId,
                                              xmlDocPtr doc,
                                              xmlNodePtr node,
                                              std::list<std::string> &errors)
{
    return true;
}

}
