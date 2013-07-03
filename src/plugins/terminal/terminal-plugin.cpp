// Plugin: terminal.
// Copyright (C) 2013 Gang Chen.

#include "terminal-plugin.hpp"
#include <gmodule.h>

namespace Samoyed
{

TerminalPlugin::TerminalPlugin(PluginManager &manager,
                               const char *id,
                               GModule *module):
    Plugin(manager, id, module)
{
}

TerminalPlugin::~TerminalPlugin()
{
}

Extension *TerminalPlugin::createExtension(const char *extensionId)
{
    return NULL;
}

}

extern "C"
{

Samoyed::Plugin *createPlugin(Samoyed::PluginManager &manager,
                              const char *id,
                              GModule *module,
                              std::string &error)
{
    return new Samoyed::TerminalPlugin(manager, id, module);
}

}
