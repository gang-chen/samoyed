// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "plugin-manager.hpp"
#include "plugin.hpp"
#include "extension-point-manager.hpp"
#include "../application.hpp"
#include <utility>
#include <libxml/xmlerror.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

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

    if (strcmp(reinterpret_cast<const char *>(node->name), "plugin") != 0)
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
        if (strcmp(reinterpret_cast<const char *>(child->name), "id") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->id = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        "name") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->name = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        "description") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->description = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        "module") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->module = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        "extension") == 0)
        {
            PluginInfo::ExtensionInfo *ext = new PluginInfo::ExtensionInfo;
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                if (strcmp(reinterpret_cast<const char *>(grandChild->name),
                           "id") == 0)
                {
                    value = reinterpret_cast<char *>(
                        xmlNodeListGetString(doc, child->children, 1));
                    if (value)
                        ext->id = value;
                    xmlFree(value);
                }
                else if (strcmp(reinterpret_cast<const char *>(grandChild->name),
                                "point") == 0)
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

    for (PluginInfo::ExtensionInfo *ext = info->extensions; ext; ext = ext->next)
    {
        Application::instance().extensionPointManager().
            registerExtension(info->id.c_str(),
                              ext->id.c_str(),
                              ext->pointId.c_str(),
                              doc,
                              ext->xmlNode);
    }

    return true;
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
}

void PluginManager::enablePlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (info->unregister || info->enabled)
        return;

    // Mark the plugin as enabled.
    info->enabled = true;

    // Activate extensions.
    for (PluginInfo::ExtensionInfo *ext = info->extensions; ext; ext = ext->next)
        Application::instance().extensionPointManager().
            activateExtension(ext->id.c_str(), ext->pointId.c_str());
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

Extension *PluginManager::loadExtension(const char *pluginId,
                                        const char *extensionId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (info->unregister || !info->enabled)
        return NULL;

    Plugin *plugin;

    // If the plugin is inactive, activate it.
    Table::const_iterator it = m_table.find(pluginId);
    if (it == m_table.end())
    {
        plugin = Plugin::activate(pluginId, info->module.c_str());
        if (!plugin)
            return NULL;
        m_table.insert(std::make_pair(plugin->id(), plugin));
    }
    else
        plugin = it->second;

    return plugin->referenceExtension(extensionId);
}

void PluginManager::scanPlugins(const char *pluginsDirName)
{
}

}
