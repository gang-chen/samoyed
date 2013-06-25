// Extension point: views.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "views-extension-point.hpp"
#include "view-extension.hpp"
#include "../application.hpp"
#include "../utilities/extension-point-manager.hpp"
#include "../utilities/plugin-manager.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

#define VIEWS "views"

namespace Samoyed
{

ViewsExtensionPoint::ViewsExtensionPoint():
    ExtensionPoint(VIEWS)
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);
}

bool ViewsExtensionPoint::registerExtension(const char *extensionId,
                                            xmlNodePtr xmlNode,
                                            std::list<std::string> &errors)
{
    return true;
}

void ViewsExtensionPoint::unregisterExtension(const char *extensionId)
{
}

}
