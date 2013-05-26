// Extension point - views.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "views-extension-point.hpp"
#include "../application.hpp"
#include "../utilities/extension-point-manager.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

ViewsExtensionPoint::ViewsExtensionPoint():
    ExtensionPoint("views")
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);
}

bool ViewsExtensionPoint::registerExtension(const char *pluginId,
                                            const char *extensionId,
                                            xmlDocPtr doc,
                                            xmlNodePtr node,
                                            std::list<std::string> &errors)
{
    return true;
}

void ViewsExtensionPoint::unregisterExtension(const char *extensionId)
{
}

bool ViewsExtensionPoint::activateExtension(char const *extensionId)
{
    return false;
}

}
