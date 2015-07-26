// Plugin: finder.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_FIND_FINDER_PLUGIN_HPP
#define SMYD_FIND_FINDER_PLUGIN_HPP

#include "plugin/plugin.hpp"
#include <set>

namespace Samoyed
{

class Widget;

namespace Finder
{

class FinderPlugin: public Plugin
{
public:
    static FinderPlugin &instance() { return *s_instance; }

    FinderPlugin(PluginManager &manager, const char *id, GModule *module);

    virtual void deactivate();

    void onTextFinderBarCreated(Widget &bar);
    void onTextFinderBarClosed(Widget &bar);

protected:
    virtual Extension *createExtension(const char *extensionId);

    virtual bool completed() const;

private:
    static FinderPlugin *s_instance;

    std::set<Widget *> m_textFinderBars;
};

}

}

#endif
