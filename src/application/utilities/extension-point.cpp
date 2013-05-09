// Extension point.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "extension-point.hpp"
#include <utility>
#include <libxml/tree.h>

namespace Samoyed
{

ExtensionPoint::Registry ExtensionPoint::s_registry;

void ExtensionPoint::registerExtensionPoint(ExtensionPoint &point)
{
    s_registry.insert(std::make_pair(point.id(), &point));
}

bool ExtensionPoint::registerExtension(const char *pluginId,
                                       xmlDocPtr doc,
                                       xmlNodePtr node,
                                       ExtensionPoint *&extensionPoint,
                                       std::string &extensionId,
                                       std::list<std::string> &errors)
{
    return true;
}

}
