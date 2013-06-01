// Plugin: terminal.
// Copyright (C) 2013 Gang Chen.

#include "terminal-plugin.hpp"
#include <stdio.h>
#include <gmodule.h>

namespace Samoyed
{

TerminalPlugin::TerminalPlugin(PluginManager &manager,
                               const char *id,
                               GModule *module):
    Plugin(manager, id, module)
{
    printf("Create terminal plugin.\n");
}

TerminalPlugin::~TerminalPlugin()
{
    printf("Destroy terminal plugin.\n");
}

Extension *TerminalPlugin::createExtension(const char *extensionId,
                                           ExtensionPoint &extensionPoint)
{
    printf("Create extension in terminal plugin.\n");
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
