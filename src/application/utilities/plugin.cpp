// Plugin.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "plugin.hpp"
#include "plugin-manager.hpp"
#include "extension.hpp"
#include <utility>
#include <map>
#include <string>
#include <glib/gi18n-lib.h>
#include <gmodule.h>

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
    m_manager(manager),
    m_id(id),
    m_module(module),
    m_nActiveExtensions(0),
    m_cached(false),
    m_nextCached(NULL),
    m_prevCached(NULL)
{
}

Plugin::~Plugin()
{
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
        char *cp = g_strdup_printf(
            _("Samoyed failed to load dynamic library \"%s\". %s.\n"),
            moduleFileName, g_module_error());
        error = cp;
        g_free(cp);
        return NULL;
    }
    PluginFactory factory;
    if (!g_module_symbol(module,
                         CREATE_PLUGIN,
                         reinterpret_cast<gpointer *>(&factory)))
    {
        char *cp = g_strdup_printf(
            _("Samoyed could not find symbol \"%s\" in dynamic library \"%s\". "
              "%s.\n"),
            CREATE_PLUGIN, moduleFileName, g_module_error());
        error = cp;
        g_free(cp);
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

Extension *Plugin::acquireExtension(const char *extensionId)
{
    ExtensionTable::const_iterator it = m_extensionTable.find(extensionId);
    if (it != m_extensionTable.end())
    {
        ++m_nActiveExtensions;
        return it->second;
    }
    Extension *ext = createExtension(extensionId);
    if (!ext)
        return NULL;
    m_extensionTable.insert(std::make_pair(ext->id(), ext));
    ++m_nActiveExtensions;
    return ext;
}

void Plugin::releaseExtension(Extension &extension)
{
    --m_nActiveExtensions;
    if (m_nActiveExtensions == 0 && completed())
        m_manager.deactivatePlugin(*this);
}

void Plugin::onCompleted()
{
    if (m_nActiveExtensions == 0)
        m_manager.deactivatePlugin(*this);
}

void Plugin::destroy()
{
    for (ExtensionTable::iterator it = m_extensionTable.begin();
         it != m_extensionTable.end();)
    {
        ExtensionTable::iterator it2 = it;
        ++it;
        Extension *ext = it2->second;
        m_extensionTable.erase(it2);
        delete ext;
    }
    GModule *module = m_module;
    delete this;
    g_module_close(module);
}

void Plugin::addToCache(Plugin *&lru, Plugin *&mru)
{
    m_cached = true;
    if (mru)
        mru->m_nextCached = this;
    else
        lru = this;
    m_prevCached = mru;
    mru = this;
}

void Plugin::removeFromCache(Plugin *&lru, Plugin *&mru)
{
    m_cached = false;
    if (m_nextCached)
        m_nextCached->m_prevCached = m_prevCached;
    else
        mru = m_prevCached;
    lru = m_nextCached;
}

}
