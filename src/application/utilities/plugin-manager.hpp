// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PLUGIN_MANAGER_HPP
#define SMYD_PLUGIN_MANAGER_HPP

#include "miscellaneous.hpp"
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class Plugin;
class Extension;
class ExtensionPoint;

class PluginManager: public boost::noncopyable
{
public:
    struct PluginInfo
    {
    public:
        struct ExtensionInfo
        {
            std::string id;
            std::string pointId;
            xmlNodePtr xmlNode;
            ExtensionInfo *next;
        };

        bool unregister;
        bool enabled;
        std::string id;
        std::string name;
        std::string description;
        std::string module;
        ExtensionInfo *extensions;
        xmlDocPtr xmlDoc;
    };

    PluginManager(const char *modulesDirName);

    /**
     * Read a plugin manifest file and register the plugin.  Register plugin
     * extensions to extension points and activate them.
     */
    bool registerPlugin(const char *pluginManifestFileName);

    /**
     * Unregister a plugin.  If the plugin is active, try to deactivate it.  The
     * plugin will be actually unregistered after it is deactivated.
     */
    void unregisterPlugin(const char *pluginId);

    const PluginInfo *findPluginInfo(const char *pluginId) const;

    /**
     * Enable a plugin and try to activate it.
     */
    void enablePlugin(const char *pluginId);

    /**
     * Disable a plugin.  If the plugin is active, try to deactivate it.
     */
    void disablePlugin(const char *pluginId);

    /**
     * Activate a plugin.
     */
    Plugin *activatePlugin(const char *pluginId);

    /**
     * Deactivate a plugin.  This function can be called by the plugin itself
     * only, when none of its extension is used and its tasks are completed.
     */
    void deactivatePlugin(Plugin &plugin);

    /**
     * Scan plugin manifest files in sub-directories of a directory and register
     * the plugins.
     */
    void scanPlugins(const char *pluignsDirName);

private:
    typedef std::map<ComparablePointer<const char *>, PluginInfo *> Registry;

    typedef std::map<ComparablePointer<const char *>, Plugin *> Table;

    void unregisterPluginInternally(PluginInfo &pluginInfo);

    const std::string m_modulesDirName;

    Registry m_registry;

    Table m_table;

    int m_nCachedPlugins;
    Plugin *m_lruCachedPlugin;
    Plugin *m_mruCachedPlugin;
};

}

#endif
