// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "plugin-manager.hpp"
#include "plugin.hpp"
#include "extension-point.hpp"
#include <libxml/tree.h>

namespace
{

const int CACHE_SIZE = 8;

}

namespace Samoyed
{

PluginManager::PluginInfo *PluginManager::PluginInfo::create(const char *dirName)
{
    PluginInfo *info = new PluginInfo(dirName);
    return info;
}

PluginManager::PluginManager():
    m_nCachedPlugins(0),
    m_lruCachedPlugin(NULL),
    m_mruCachedPlugin(NULL)
{
}

void PluginManager::scanPlugins()
{
}

void PluginManager::enablePlugin(const char *pluginId)
{
    PluginInfo *info = findPluginInfo(pluginId);
    if (info->enabled())
        return;

    info->enable();

    // Activate extensions.
    info->activateExtensions();
}

void PluginManager::disablePlugin(const char *pluginId)
{
    PluginInfo *info = findPluginInfo(pluginId);
    if (!info->enabled())
        return;

    // Mark the plugin as disabled.
    info->disable();

    // If the plugin is active, force to deactivate it.
}

Extension *PluginManager::loadExtension(const char *pluginId,
                                        const char *extensionId)
{
    PluginInfo *info = findPluginInfo(pluginId);
    if (!info->enabled())
        return NULL;

    // If the plugin in inactive, activate it.
    Plugin *plugin = Plugin::activate(pluginId);

    return plugin->referenceExtension(extensionId);
}

}
