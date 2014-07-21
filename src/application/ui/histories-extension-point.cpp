// Extension point: histories.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "histories-extension-point.hpp"
#include "histories-extension.hpp"
#include "application.hpp"
#include "utilities/extension-point-manager.hpp"
#include "utilities/plugin-manager.hpp"
#include <list>
#include <map>
#include <string>
#include <utility>
#include <libxml/tree.h>

#define HISTORIES "histories"

namespace Samoyed
{

HistoriesExtensionPoint::HistoriesExtensionPoint():
    ExtensionPoint(HISTORIES)
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);
}

HistoriesExtensionPoint::~HistoriesExtensionPoint()
{
    for (ExtensionTable::iterator it = m_extensions.begin();
         it != m_extensions.end();)
    {
        ExtensionTable::iterator it2 = it;
        ++it;
        ExtensionInfo *ext = it2->second;
        m_extensions.erase(it2);
        delete ext;
    }

    Application::instance().extensionPointManager().
        unregisterExtensionPoint(*this);
}

bool HistoriesExtensionPoint::registerExtension(
    const char *extensionId,
    xmlNodePtr xmlNode,
    std::list<std::string> &errors)
{
    // Parse the extension.
    ExtensionInfo *extInfo = new ExtensionInfo(extensionId);
    m_extensions.insert(std::make_pair(extInfo->id.c_str(), extInfo));

    // Install histories.
    HistoriesExtension *ext = static_cast<HistoriesExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extensionId));
    if (!ext)
    {
        unregisterExtension(extensionId);
        return false;
    }
    ext->installHistories();
    ext->release();
    return true;
}

void HistoriesExtensionPoint::unregisterExtension(const char *extensionId)
{
    ExtensionInfo *ext = m_extensions[extensionId];
    m_extensions.erase(extensionId);
    delete ext;
}

}
