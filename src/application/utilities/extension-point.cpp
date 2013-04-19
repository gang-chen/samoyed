// Extension point.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "extension-point.hpp"

namespace Samoyed
{

ExtensionPoint::Registry ExtensionPoint::s_registry;

bool ExtensionPoint::addExtension(const char *pluginId,
                                  xmlDocPtr doc,
                                  xmlNodePtr node,
                                  std::list<std::string> &errors)
{
    return true;
}

bool ExtensionPoint::registerExtensionPoint(ExtensionPoint *point)
{
    return true;
}

}
