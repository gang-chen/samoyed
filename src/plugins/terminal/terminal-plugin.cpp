// Plugin: terminal.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "terminal-plugin.hpp"
#include "terminal-view-extension.hpp"
#include <gmodule.h>

namespace Samoyed
{

TerminalPlugin::TerminalPlugin(PluginManager &manager,
                               const char *id,
                               GModule *module):
    Plugin(manager, id, module),
    m_viewCount(0)
{
}

TerminalPlugin::~TerminalPlugin()
{
}

Extension *TerminalPlugin::createExtension(const char *extensionId)
{
    return new TerminalViewExtension(extensionId, *this);
}

void TerminalPlugin::onViewCreated()
{
    ++m_viewCount;
}

void TerminalPlugin::onViewClosed()
{
    --m_viewCount;
    if (m_viewCount == 0)
        onCompleted();
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
