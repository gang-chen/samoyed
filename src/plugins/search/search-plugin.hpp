// Plugin: search.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_SRCH_SEARCH_PLUGIN_HPP
#define SMYD_SRCH_SEARCH_PLUGIN_HPP

#include "utilities/plugin.hpp"

namespace Samoyed
{

namespace Search
{

class SearchPlugin: public Plugin
{
public:
    static SearchPlugin &instance() { return *s_instance; }

    SearchPlugin(PluginManager &manager, const char *id, GModule *module);

    virtual void deactivate();

protected:
    virtual Extension *createExtension(const char *extensionId);

    virtual bool completed() const;

private:
    static SearchPlugin *s_instance;
};

}

}

#endif
