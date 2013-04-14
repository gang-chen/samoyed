// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PLUGIN_MANAGER_HPP
#define SMYD_PLUGIN_MANAGER_HPP

#include <map>

namespace Samoyed
{

class Plugin;

class PluginManager
{
public:
    /**
     * Scan plugins in the plugins directory.  Read in plugin manifest files.
     * Add plugin extensions to extension points.
     */
    void scanPlugins();

    Plugin *findPlugin(const char *pluginId);

    Plugin *activatePlugin(const char *pluginId);

private:
    std::map<std::string, Plugin *> m_pluginTable;
};

}

#endif
