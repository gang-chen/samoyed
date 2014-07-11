// Plugin: finder.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "finder-plugin.hpp"
#include "find-text-action-extension.hpp"
#include "ui/widget.hpp"
#include <gmodule.h>

namespace Samoyed
{

namespace Finder
{

FinderPlugin *FinderPlugin::s_instance = NULL;

FinderPlugin::FinderPlugin(PluginManager &manager,
                           const char *id,
                           GModule *module):
    Plugin(manager, id, module)
{
    s_instance = this;
}

Extension *FinderPlugin::createExtension(const char *extensionId)
{
    if (strcmp(extensionId, "finder/find-text") == 0)
        return new FindTextActionExtension(extensionId, *this);
    return NULL;
}

void FinderPlugin::onTextFinderBarCreated(Widget &bar)
{
    m_bars.insert(&bar);
}

void FinderPlugin::onTextFinderBarDestroyed(Widget &bar)
{
    m_bars.erase(&bar);
    if (completed())
        onCompleted();
}

bool FinderPlugin::completed() const
{
    return m_bars.empty();
}

void FinderPlugin::deactivate()
{
    while (!m_bars.empty())
        (*m_bars.begin())->close();
}

}

}

extern "C"
{

Samoyed::Plugin *createPlugin(Samoyed::PluginManager *manager,
                              const char *id,
                              GModule *module,
                              std::string *error)
{
    return new Samoyed::Finder::FinderPlugin(*manager, id, module);
}

}
