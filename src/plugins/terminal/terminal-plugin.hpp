// Plugin: terminal.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TERM_TERMINAL_PLUGIN_HPP
#define SMYD_TERM_TERMINAL_PLUGIN_HPP

#include "utilities/plugin.hpp"
#include <set>

namespace Samoyed
{

class Widget;

namespace Terminal
{

class TerminalPlugin: public Plugin
{
public:
    static TerminalPlugin &instance() { return *s_instance; }

    TerminalPlugin(PluginManager &manager, const char *id, GModule *module);

    virtual void deactivate();

    void onViewCreated(Widget &view);
    void onViewClosed(Widget &view);

protected:
    virtual Extension *createExtension(const char *extensionId);

    virtual bool completed() const;

private:
    static TerminalPlugin *s_instance;

    std::set<Widget *> m_views;
};

}

}

#endif
