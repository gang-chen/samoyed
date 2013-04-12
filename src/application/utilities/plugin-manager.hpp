// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PLUGIN_MANAGER_HPP
#define SMYD_PLUGIN_MANAGER_HPP

namespace Samoyed
{

class PluginManager
{
public:
    PluginManager();

    ~PluginManager();

    /**
     * Scan plugins in the plugins directory.  Read in the plugin manifest
     * files.
     */
    void scanPlugins();
};

}

#endif
