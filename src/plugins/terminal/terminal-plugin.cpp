// Plugin: terminal.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "terminal-plugin.hpp"
#include "terminal-view-extension.hpp"
#include "ui/widget.hpp"
#include <algorithm>
#include <gmodule.h>

namespace Samoyed
{

namespace Terminal
{

TerminalPlugin::TerminalPlugin(PluginManager &manager,
                               const char *id,
                               GModule *module):
    Plugin(manager, id, module)
{
}

Extension *TerminalPlugin::createExtension(const char *extensionId)
{
    return new TerminalViewExtension(extensionId, *this);
}

void TerminalPlugin::onViewCreated(Widget &view)
{
    m_views.push_back(&view);
}

void TerminalPlugin::onViewClosed(Widget &view)
{
    m_views.erase(std::remove(m_views.begin(), m_views.end(), &view),
                  m_views.end());
    if (completed())
        onCompleted();
}

bool TerminalPlugin::completed() const
{
    return m_views.empty();
}

void TerminalPlugin::deactivate()
{
    while (!m_views.empty())
        m_views.front()->close();
}

}

}

extern "C"
{

Samoyed::Plugin *createPlugin(Samoyed::PluginManager &manager,
                              const char *id,
                              GModule *module,
                              std::string &error)
{
    return new Samoyed::Terminal::TerminalPlugin(manager, id, module);
}

}
