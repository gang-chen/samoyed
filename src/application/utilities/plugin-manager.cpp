// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "plugin-manager.hpp"
#include "plugin.hpp"
#include "extension-point.hpp"

namespace
{

const int CACHE_SIZE = 8;

}

namespace Samoyed
{

PluginManager::PluginManager():
    m_nCachedPlugins(0),
    m_lruCachedPlugin(NULL),
    m_mruCachedPlugin(NULL)
{
}

void PluginManager::scanPlugins()
{
}

bool PluginManager::enablePlugin(const char *pluginId)
{
    PluginData *data = findPluginData(pluginId);
    if (!data)
        return false;

    if (data->m_enabled)
        return true;

    // Mark the plugin as enabled.
    data->m_enabled = true;

    // Read the plugin manifest file and register extensions.
    if (!readPluginManifestFile(data->m_dirName.c_str()))
        data->m_enabled = false;

    return data->m_enabled;
}

bool PluginManager::disablePlugin(const char *pluginId)
{
    PluginData *data = findPluginData(pluginId);
    if (!data)
        return false;

    if (!data->m_enabled)
        return true;

    // If the plugin is active, try to deactivate it.

    // Unregister the extensions from extension points.

    // Mark the plugin as disabled.
    data->m_enabled = false;
    return true;
}

void PluginManager::activatePlugins()
{
}

}
