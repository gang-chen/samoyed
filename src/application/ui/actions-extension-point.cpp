// Extension point - actions.
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

namespace Samoyed
{

ActionsExtensionPoint::ActionsExtensionPoint():
    ExtensionPoint("actions")
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);
}

bool ActionsExtensionPoint::registerExtension(const char *pluginId,
                                              const char *extensionId,
                                              xmlDocPtr doc,
                                              xmlNodePtr node,
                                              std::list<std::string> &errors)
{
    return true;
}

void ActionsExtensionPoint::unregisterExtension(const char *extensionId)
{
}

bool ActionsExtensionPoint::activateExtension(char const *extensionId)
{
    return false;
}

}
