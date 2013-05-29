// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "plugin-manager.hpp"
#include "plugin.hpp"
#include "extension-point-manager.hpp"
#include "extension-point.hpp"
#include "../application.hpp"
#include <utility>
#include <glib.h>
#include <gmodule.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define PLUGIN "plugin"
#define ID "id"
#define NAME "name"
#define DESCRIPTION "description"
#define MODULE "module"
#define EXTENSION "extension"
#define POINT "point"

namespace
{

const int CACHE_SIZE = 8;

}

namespace Samoyed
{

PluginManager::PluginManager(const char *modulesDirName):
    m_modulesDirName(modulesDirName),
    m_nCachedPlugins(0),
    m_lruCachedPlugin(NULL),
    m_mruCachedPlugin(NULL)
{
}

const PluginManager::PluginInfo *
PluginManager::findPluginInfo(const char *pluginId) const
{
    Registry::const_iterator it = m_registry.find(pluginId);
    if (it == m_registry.end())
        return NULL;
    return it->second;
}

bool PluginManager::registerPlugin(const char *pluginManifestFileName)
{
    xmlDocPtr doc = xmlParseFile(pluginManifestFileName);
    if (!doc)
    {
        return false;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node)
    {
        xmlFreeDoc(doc);
        return false;
    }

    if (strcmp(reinterpret_cast<const char *>(node->name), PLUGIN) != 0)
    {
        xmlFreeDoc(doc);
        return false;
    }

    PluginInfo *info = new PluginInfo;
    info->unregister = false;
    info->enabled = true;
    info->extensions = NULL;
    info->xmlDoc = doc;

    char *value;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name), ID) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->id = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        NAME) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->name = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        DESCRIPTION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->description = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        MODULE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->module = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        EXTENSION) == 0)
        {
            PluginInfo::ExtensionInfo *ext = new PluginInfo::ExtensionInfo;
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                if (strcmp(reinterpret_cast<const char *>(grandChild->name),
                           ID) == 0)
                {
                    value = reinterpret_cast<char *>(
                        xmlNodeListGetString(doc, child->children, 1));
                    if (value)
                        ext->id = info->id + '.' + value;
                    xmlFree(value);
                }
                else if (strcmp(reinterpret_cast<const char *>(grandChild->name),
                                POINT) == 0)
                {
                    value = reinterpret_cast<char *>(
                        xmlNodeListGetString(doc, child->children, 1));
                    if (value)
                        ext->pointId = value;
                    xmlFree(value);
                }
            }
            if (ext->id.empty())
            {
                delete ext;
            }
            else if (ext->pointId.empty())
            {
                delete ext;
            }
            else
            {
                ext->xmlNode = child;
                ext->next = info->extensions;
                info->extensions = ext;
            }
        }
    }

    if (!info->extensions)
    {
        xmlFreeDoc(info->xmlDoc);
        delete info;
        return false;
    }

    m_registry.insert(std::make_pair(info->id.c_str(), info));

    for (PluginInfo::ExtensionInfo *ext = info->extensions;
         ext;
         ext = ext->next)
    {
        Application::instance().extensionPointManager().
            registerExtension(ext->id.c_str(),
                              ext->pointId.c_str(),
                              doc,
                              ext->xmlNode);
    }

    return true;
}

void PluginManager::unregisterPluginInternally(PluginInfo &pluginInfo)
{
    for (PluginInfo::ExtensionInfo *ext = pluginInfo.extensions;
         ext;
         ext = ext->next)
    {
        Application::instance().extensionPointManager().
            unregisterExtension(ext->id.c_str(),
                                ext->pointId.c_str());
    }
    m_registry.erase(pluginInfo.id.c_str());
    delete &pluginInfo;
}

void PluginManager::unregisterPlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (info->unregister)
        return;

    // Mark the plugin as to be unregistered.
    info->unregister = true;

    // If the plugin is active, try to deactivate it.
    Table::const_iterator it = m_table.find(pluginId);
    if (it != m_table.end())
        it->second->deactivate();
    else
        unregisterPluginInternally(*info);
}

void PluginManager::enablePlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (info->unregister || info->enabled)
        return;

    // Mark the plugin as enabled.
    info->enabled = true;

    // Activate extensions.
    for (PluginInfo::ExtensionInfo *ext = info->extensions;
         ext;
         ext = ext->next)
    {
        ExtensionPoint *point = Application::instance().extensionPointManager().
            extensionPoint(ext->pointId.c_str());
        if (point)
            point->onExtensionEnabled(ext->id.c_str());
    }
}

void PluginManager::disablePlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (info->unregister || !info->enabled)
        return;

    // Mark the plugin as disabled.
    info->enabled = false;

    // If the plugin is active, try to deactivate it.
    Table::const_iterator it = m_table.find(pluginId);
    if (it != m_table.end())
        it->second->deactivate();
}

Plugin *PluginManager::activatePlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (info->unregister || !info->enabled)
        return NULL;

    char *module = g_module_build_path(m_modulesDirName.c_str(),
                                       info->module.c_str());
    std::string error;
    Plugin *plugin = Plugin::activate(pluginId, module, error);
    g_free(module);
    if (!plugin)
        return NULL;
    m_table.insert(std::make_pair(plugin->id(), plugin));
    return plugin;
}

void PluginManager::deactivatePlugin(Plugin &plugin)
{
    m_table.erase(plugin.id());
    std::string pluginId(plugin.id());
    delete &plugin;
    PluginInfo *info = m_registry.find(pluginId.c_str())->second;
    if (info->unregister)
        unregisterPluginInternally(*info);
}

void PluginManager::scanPlugins(const char *pluginsDirName)
{
}

}
