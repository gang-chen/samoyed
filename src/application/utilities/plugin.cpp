// Plugin.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "plugin.hpp"
#include "plugin-manager.hpp"
#include <gmodule.h>

namespace Samoyed
{

Plugin::Plugin(const char *id):
    m_id(id),
    m_nextCached(NULL),
    m_prevCached(NULL)
{
}

Plugin::~Plugin()
{
}

Plugin *Plugin::activate(const char *id, const char *module)
{
    return NULL;
}

bool Plugin::deactivate()
{
    return false;
}

Extension *Plugin::referenceExtension(const char *extensionId)
{
    return NULL;
}

void Plugin::onExtensionReleased(Extension &extension)
{
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
