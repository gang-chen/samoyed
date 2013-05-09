// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PLUGIN_MANAGER_HPP
#define SMYD_PLUGIN_MANAGER_HPP

#include "miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <boost/thread/mutex.hpp>

namespace Samoyed
{

class Plugin;
class Extension;
class ExtensionPoint;

class PluginManager: public boost::noncopyable
{
public:
    class PluginInfo
    {
    public:
        struct ExtensionInfo
        {
            std::string m_extensionId;
            ExtensionPoint *m_extensionPoint;
        };

        static PluginInfo *create(const char *dirName);

        bool enabled() const { return m_enabled; }
        void enable() { m_enabled = true; }
        void disable() { m_enabled = false; }

        const char *dirName() const { return m_dirName.c_str(); }
        const char *id() const { return m_id.c_str(); }
        const char *name() const { return m_name.c_str(); }
        const char *description() const { return m_description.c_str(); }
        const char *libraryName() const { return m_libraryName.c_str(); }

        const std::list<ExtensionInfo> &extensions() const
        { return m_extensions; }
        void activateExtensions();

    private:
        PluginInfo(const char *dirName): m_enabled(true), m_dirName(dirName) {}

        bool m_enabled;
        std::string m_dirName;
        std::string m_id;
        std::string m_name;
        std::string m_description;
        std::string m_libraryName;
        std::list<ExtensionInfo> m_extensions;
    };

    typedef std::map<ComparablePointer<const char *>, PluginInfo>
        PluginRegistry;

    PluginManager();

    /**
     * Scan plugins in the plugins directory.  Read plugin manifest files.
     * Register plugin extensions to extension points and activate them.
     */
    void scanPlugins();

    const PluginRegistry &pluginRegistry() const { return m_pluginRegistry; }

    PluginInfo *findPluginInfo(const char *pluginId) const;

    void enablePlugin(const char *pluginId);

    void disablePlugin(const char *pluginId);

    /**
     * Load an extension from a plugin.  If the plugin is inactive, activate it
     * first.
     */
    Extension *loadExtension(const char *pluginId, const char *extensionId);

private:
    typedef std::map<ComparablePointer<const char *>, Plugin *> PluginTable;

    /**
     * Deactivate a plugin.  This function can be called by the plugin itself
     * only, when none of its extension is used and its task is completed.
     */
    void deactivatePlugin(Plugin &plugin);

    PluginRegistry m_pluginRegistry;

    PluginTable m_pluginTable;

    int m_nCachedPlugins;
    Plugin *m_lruCachedPlugin;
    Plugin *m_mruCachedPlugin;

    mutable boost::mutex m_mutex;

    friend class Plugin;
};

}

#endif
