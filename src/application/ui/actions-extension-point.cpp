// Extension point: actions.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "actions-extension-point.hpp"
#include "../application.hpp"
#include "../utilities/extension-point-manager.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

#define ACTIONS "actions"

namespace Samoyed
{

ActionsExtensionPoint::ActionsExtensionPoint():
    ExtensionPoint(ACTIONS)
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);
}

bool ActionsExtensionPoint::registerExtension(const char *extensionId,
                                              xmlDocPtr xmlDoc,
                                              xmlNodePtr xmlNode,
                                              std::list<std::string> &errors)
{
    return true;
}

void ActionsExtensionPoint::unregisterExtension(const char *extensionId)
{
}

void ActionsExtensionPoint::onExtensionEnabled(char const *extensionId)
{
}

}
