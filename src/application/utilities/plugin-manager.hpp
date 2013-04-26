// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PLUGIN_MANAGER_HPP
#define SMYD_PLUGIN_MANAGER_HPP

#include "miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/utility.hpp>

namespace Samoyed
{

class Plugin;

class PluginManager: public boost::noncopyable
{
public:
    struct PluginData
    {
        struct Extension
        {
            std::string m_extensionId;
            std::string m_extensionPointId;
        };

        std::string m_id;
        std::string m_name;
        std::string m_description;
        std::string m_libraryName;
        std::string m_dirName;
        bool m_enabled;
        std::list<Extension> m_extensions;
    };

    typedef std::map<ComparablePointer<const char *>, PluginData>
        PluginRegistry;

    PluginManager();

    /**
     * Scan plugins in the plugins directory.  Read plugin manifest files.
     * Register plugins.  Register plugin extensions to extension points.
     */
    void scanPlugins();

    const PluginRegistry &pluginRegistry() const { return m_pluginRegistry; }

    PluginData *findPluginData(const char *pluginId) const;

    bool enablePlugin(const char *pluginId);

    bool disablePlugin(const char *pluginId);

    Plugin *findPlugin(const char *pluginId) const;

    Plugin *activatePlugin(const char *pluginId);

    bool deactivatePlugin(Plugin &plugin);

private:
    typedef std::map<ComparablePointer<const char *>, Plugin *> PluginTable;

    bool readPluginManifestFile(const char *pluginDirName);

    PluginRegistry m_pluginRegistry;

    PluginTable m_pluginTable;

    int m_nCachedPlugins;
    Plugin *m_lruCachedPlugin;
    Plugin *m_mruCachedPlugin;
};

}

#endif
