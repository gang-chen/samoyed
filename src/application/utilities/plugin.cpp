// Plugin.
// Copyright (C) 2013 Gang Chen.

/*
UNIT TEST BUILD
g++ plugin.cpp -DSMYD_PLUGIN_UNIT_TEST `pkg-config --cflags --libs gmodule-2.0`\
 -Werror -Wall -fPIC -DPIC -shared -Wl,-soname,libhelloworld.so -o
libhelloworld.so
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "plugin.hpp"
#include "plugin-manager.hpp"
#include "extension.hpp"
#include <utility>
#include <map>
#include <string>
#include <gmodule.h>

#ifndef SMYD_PLUGIN_UNIT_TEST

#define CREATE_PLUGIN "createPlugin"

namespace
{

typedef Samoyed::Plugin *(*PluginFactory)(Samoyed::PluginManager &manager,
                                          const char *id,
                                          GModule *module,
                                          std::string &error);

}

namespace Samoyed
{

Plugin::Plugin(PluginManager &manager, const char *id, GModule *module):
    m_activeExtensionCount(0),
    m_manager(manager),
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

Plugin *Plugin::activate(PluginManager &manager,
                         const char *id,
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
    Plugin *plugin = factory(manager, id, module, error);
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

Extension *Plugin::acquireExtension(const char *extensionId,
                                    ExtensionPoint &extensionPoint)
{
    ExtensionTable::const_iterator it = m_extensionTable.find(extensionId);
    if (it != m_extensionTable.end())
    {
        ++m_activeExtensionCount;
        return it->second;
    }
    Extension *ext = createExtension(extensionId, extensionPoint);
    if (!ext)
        return NULL;
    m_extensionTable.insert(std::make_pair(ext->id(), ext));
    ++m_activeExtensionCount;
    return ext;
}

void Plugin::releaseExtension(Extension &extension)
{
    --m_activeExtensionCount;
    if (!active())
        m_manager.deactivatePlugin(*this);
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

#else

class HelloWorldPlugin: public Samoyed::Plugin
{
public:
    HelloWorldPlugin(Samoyed::PluginManager &manager,
                     const char *id,
                     GModule *module):
        Samoyed::Plugin(manager, id, module)
    {}
}

Samoyed::Plugin *createPlugin(Samoyed::PluginManager &manager,
                              const char *id,
                              GModule *module,
                              std::string &error)
{
}

#endif
