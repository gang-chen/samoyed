// Extension point manager.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "extension-point-manager.hpp"
#include "extension-point.hpp"
#include <utility>
#include <list>
#include <string>

namespace Samoyed
{

void ExtensionPointManager::registerExtensionPoint(ExtensionPoint &extPoint)
{
    m_registry.insert(std::make_pair(extPoint.id(), &extPoint));

    std::list<std::string> errors;
    ExtensionRegistry::const_iterator it =
        m_extensionRegistry.find(extPoint.id());
    if (it != m_extensionRegistry.end())
    {
        for (ExtensionInfo *ext = it->second; ext; ext = ext->next)
            extPoint.registerExtension(ext->pluginId.c_str(),
                                       ext->extensionId.c_str(),
                                       ext->xmlDoc,
                                       ext->xmlNode,
                                       errors);
    }
}

void ExtensionPointManager::unregisterExtensionPoint(ExtensionPoint &extPoint)
{
}

void ExtensionPointManager::registerExtension(const char *pluginId,
                                              const char *extensionId,
                                              const char *extensionPointId,
                                              xmlDocPtr doc,
                                              xmlNodePtr node)
{
}

void ExtensionPointManager::unregisterExtension(const char *extensionId,
                                                const char *extensionPointId)
{
}

void ExtensionPointManager::activateExtension(const char *extensionId,
                                              const char *extensionPointId)
{
}

}
