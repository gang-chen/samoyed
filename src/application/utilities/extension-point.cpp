// Extension point.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "extension-point.hpp"

namespace Samoyed
{

ExtensionPoint::Registry ExtensionPoint::s_registry;

void ExtensionPoint::registerExtensionPoint(ExtensionPoint *point)
{
}

bool ExtensionPoint::registerExtension(const char *pluginId,
                                       xmlDocPtr doc,
                                       xmlNodePtr node,
                                       std::string &extensionId,
                                       std::list<std::string> &errors)
{
    return true;
}

}
