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
        for (ExtensionInfo *ext = it->second; ext; ext = ext->next)
            extPoint.registerExtension(ext->extensionId.c_str(),
                                       ext->xmlNode,
                                       NULL);
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

void ExtensionPointManager::registerExtension(const char *extensionId,
                                              const char *extensionPointId,
                                              xmlNodePtr xmlNode)
{
    ExtensionInfo *ext = new ExtensionInfo;
    ext->extensionId = extensionId;
    ext->xmlNode = xmlNode;
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
        it2->second->registerExtension(extensionId,
                                       xmlNode,
                                       NULL);
}

void ExtensionPointManager::unregisterExtension(const char *extensionId,
                                                const char *extensionPointId)
{
    Registry::const_iterator it = m_registry.find(extensionPointId);
    if (it != m_registry.end())
        it->second->unregisterExtension(extensionId);

    ExtensionRegistry::iterator it2 =
        m_extensionRegistry.find(extensionPointId);
    for (ExtensionInfo **ext = &it2->second; *ext; ext = &(*ext)->next)
    {
        if ((*ext)->extensionId == extensionId)
        {
            ExtensionInfo *t = *ext;
            *ext = t->next;
            delete t;
            break;
        }
    }
    if (!it2->second)
        m_extensionRegistry.erase(it2);
}

}
