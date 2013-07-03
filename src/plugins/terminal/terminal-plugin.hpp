// Plugin: terminal.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TERMINAL_PLUGIN_HPP
#define SMYD_TERMINAL_PLUGIN_HPP

#include "../../application/utilities/plugin.hpp"

namespace Samoyed
{

class TerminalPlugin: public Plugin
{
public:
    TerminalPlugin(PluginManager &manager, const char *id, GModule *module);

protected:
    virtual ~TerminalPlugin();

    virtual Extension *createExtension(const char *extensionId);

    virtual bool completed() const { return true; }
};

}

#endif
