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

    ExtensionRegistry::const_iterator it =
        m_extensionRegistry.find(extPoint.id());
    if (it != m_extensionRegistry.end())
    {
        std::list<std::string> errors;
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
    ExtensionRegistry::const_iterator it =
        m_extensionRegistry.find(extPoint.id());
    if (it != m_extensionRegistry.end())
    {
        for (ExtensionInfo *ext = it->second; ext; ext = ext->next)
            extPoint.unregisterExtension(ext->extensionId.c_str());
    }

    m_registry.erase(extPoint.id());
}

void ExtensionPointManager::registerExtension(const char *pluginId,
                                              const char *extensionId,
                                              const char *extensionPointId,
                                              xmlDocPtr doc,
                                              xmlNodePtr node)
{
    ExtensionInfo *ext = new ExtensionInfo;
    ext->pluginId = pluginId;
    ext->extensionId = extensionId;
    ext->xmlDoc = doc;
    ext->xmlNode = node;
    ExtensionRegistry::iterator it = m_extensionRegistry.find(extensionPointId);
    if (it != m_extensionRegistry.end())
    {
        ext->next = it->second;
        it->second = ext;
    }
    else
    {
        ext->next = NULL;
        m_extensionRegistry.insert(std::make_pair(extensionPointId, ext));
    }

    Registry::const_iterator it2 = m_registry.find(extensionPointId);
    if (it2 != m_registry.end())
    {
        std::list<std::string> errors;
        it2->second->registerExtension(pluginId,
                                       extensionId,
                                       doc,
                                       node,
                                       errors);
    }
}

void ExtensionPointManager::unregisterExtension(const char *extensionId,
                                                const char *extensionPointId)
{
    Registry::const_iterator it = m_registry.find(extensionPointId);
    if (it != m_registry.end())
        it->second->unregisterExtension(extensionId);

}

void ExtensionPointManager::activateExtension(const char *extensionId,
                                              const char *extensionPointId)
{
    Registry::const_iterator it = m_registry.find(extensionPointId);
    if (it != m_registry.end())
        it->second->activateExtension(extensionId);
}

}
