// Plugin: search.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "search-plugin.hpp"
#include <gmodule.h>

namespace Samoyed
{

namespace Search
{

SearchPlugin *SearchPlugin::s_instance = NULL;

SearchPlugin::SearchPlugin(PluginManager &manager,
                           const char *id,
                           GModule *module):
    Plugin(manager, id, module)
{
    s_instance = this;
}

Extension *SearchPlugin::createExtension(const char *extensionId)
{
    return NULL;
}

bool SearchPlugin::completed() const
{
    return true;
}

void SearchPlugin::deactivate()
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
    return new Samoyed::Search::SearchPlugin(*manager, id, module);
}

}
