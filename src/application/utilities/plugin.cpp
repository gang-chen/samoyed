// Plugin.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "plugin.hpp"
#include "plugin-manager.hpp"
#include "extension.hpp"
#include "../application.hpp"
#include <utility>
#include <map>
#include <string>
#include <gmodule.h>

#define CREATE_PLUGIN "create_plugin"

namespace
{

typedef Samoyed::Plugin *(*PluginFactory)(const char *id,
                                          GModule *module,
                                          std::string &error);

}

namespace Samoyed
{

Plugin::Plugin(const char *id, GModule *module):
    m_id(id),
    m_module(module),
    m_nextCached(NULL),
    m_prevCached(NULL)
{
}

Plugin::~Plugin()
{
    g_module_close(m_module);
}

Plugin *Plugin::activate(const char *id,
                         const char *moduleFileName,
                         std::string &error)
{
    GModule *module = g_module_open(moduleFileName,
                                    static_cast<GModuleFlags>(0));
    if (!module)
    {
        return NULL;
    }
    PluginFactory factory;
    if (!g_module_symbol(module,
                         CREATE_PLUGIN,
                         reinterpret_cast<gpointer *>(&factory)))
    {
        g_module_close(module);
        return NULL;
    }
    Plugin *plugin = factory(id, module, error);
    if (!plugin)
    {
        g_module_close(module);
        return NULL;
    }
    return plugin;
}

bool Plugin::deactivate()
{
    return false;
}

void Plugin::onExtensionDestroyed(Extension &extension)
{
    m_extensionTable.erase(extension.id());
    if (m_extensionTable.empty() && !used())
        Application::instance().pluginManager().deactivatePlugin(*this);
}

void Plugin::addToCache(Plugin *&lru, Plugin *&mru)
{
    if (mru)
        mru->m_nextCached = this;
    else
        lru = this;
    m_prevCached = mru;
    mru = this;
}

void Plugin::removeFromCache(Plugin *&lru, Plugin *&mru)
{
    if (m_nextCached)
        m_nextCached->m_prevCached = m_prevCached;
    else
        mru = m_prevCached;
    lru = m_nextCached;
}

}
