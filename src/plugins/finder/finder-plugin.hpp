// Plugin: finder.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_FIND_SEARCH_PLUGIN_HPP
#define SMYD_FIND_SEARCH_PLUGIN_HPP

#include "utilities/plugin.hpp"

namespace Samoyed
{

namespace Finder
{

class FinderPlugin: public Plugin
{
public:
    static FinderPlugin &instance() { return *s_instance; }

    FinderPlugin(PluginManager &manager, const char *id, GModule *module);

    virtual void deactivate();

protected:
    virtual Extension *createExtension(const char *extensionId);

    virtual bool completed() const;

private:
    static FinderPlugin *s_instance;
};

}

}

#endif
