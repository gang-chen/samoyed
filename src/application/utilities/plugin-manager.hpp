// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PLUGIN_MANAGER_HPP
#define SMYD_PLUGIN_MANAGER_HPP

#include "miscellaneous.hpp"
#include <map>
#include <boost/utility.hpp>

namespace Samoyed
{

class Plugin;

class PluginManager: public boost::noncopyable
{
public:
    /**
     * Scan plugins in the plugins directory.  Read in plugin manifest files.
     * Add plugin extensions to extension points.
     */
    void scanPlugins();

    bool isPluginEnabled(const char *pluginId);

    bool enablePlugin(const char *pluginId);

    bool disablePlugin(const char *pluginId);

    Plugin *findPlugin(const char *pluginId);

    Plugin *activatePlugin(const char *pluginId);

    void deactivatePlugin(const char *pluginId);

private:
    typedef std::map<ComparablePointer<const char *>, Plugin *> PluginTable;

    PluginTable m_pluginTable;

    int m_nCachedPlugins;
    Plugin *m_lruCachedPlugin;
    Plugin *m_mruCachedPlugin;
};

}

#endif
