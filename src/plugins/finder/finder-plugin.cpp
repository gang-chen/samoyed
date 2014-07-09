// Plugin: finder.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "finder-plugin.hpp"
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
    return NULL;
}

bool FinderPlugin::completed() const
{
    return true;
}

void FinderPlugin::deactivate()
{
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
