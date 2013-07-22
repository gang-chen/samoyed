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

class ExtensionPointManager;
class Plugin;
class Extension;

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
            ExtensionInfo(): xmlNode(NULL) {}
        };

        std::string directoryName;
        std::string id;
        std::string name;
        std::string description;
        std::string module;
        ExtensionInfo *extensions;
        xmlDocPtr xmlDoc;
    };

    typedef std::map<ComparablePointer<const char>, PluginInfo *> Registry;

    PluginManager(ExtensionPointManager &extensionPointMgr,
                  const char *modulesDirName,
                  int cacheSize);

    ~PluginManager();

    /**
     * Read a plugin manifest file and register the plugin.  Register plugin
     * extensions to extension points and activate them if desired.
     */
    bool registerPlugin(const char *pluginManifestFileName);

    /**
     * Unregister a plugin.  If the plugin is active, first deactivate it.
     */
    void unregisterPlugin(const char *pluginId);

    const PluginInfo *findPluginInfo(const char *pluginId) const;

    const Registry &pluginRegistry() const { return m_registry; }

    /**
     * Enable a plugin and activate it if desired.
     */
    void enablePlugin(const char *pluginId);

    /**
     * Disable a plugin.  If the plugin is active, first deactivate it.
     */
    void disablePlugin(const char *pluginId);

    /**
     * Acquire an extension.  If the plugin that provides the extension is
     * inactive, activate it.
     */
    Extension *acquireExtension(const char *extensionId);

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
    typedef std::map<ComparablePointer<const char>, Plugin *> Table;

    void registerPluginExtensions(PluginInfo &pluginInfo);
    void unregisterPluginExtensions(PluginInfo &pluginInfo);

    void unregisterPluginInternally(PluginInfo &pluginInfo);

    ExtensionPointManager &m_extensionPointManager;

    const std::string m_modulesDirName;

    Registry m_registry;

    Table m_table;

    const int m_cacheSize;
    int m_nCachedPlugins;
    Plugin *m_lruCachedPlugin;
    Plugin *m_mruCachedPlugin;
};

}

#endif
