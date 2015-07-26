// Plugin: terminal.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "terminal-plugin.hpp"
#include "terminal-view-extension.hpp"
#include "widget/widget.hpp"
#include <string.h>

namespace Samoyed
{

namespace Terminal
{

TerminalPlugin *TerminalPlugin::s_instance = NULL;

TerminalPlugin::TerminalPlugin(PluginManager &manager,
                               const char *id,
                               GModule *module):
    Plugin(manager, id, module)
{
    s_instance = this;
}

Extension *TerminalPlugin::createExtension(const char *extensionId)
{
    if (strcmp(extensionId, "terminal/view") == 0)
        return new TerminalViewExtension(extensionId, *this);
    return NULL;
}

void TerminalPlugin::onViewCreated(Widget &view)
{
    m_views.insert(&view);
}

void TerminalPlugin::onViewClosed(Widget &view)
{
    m_views.erase(&view);
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
        (*m_views.begin())->close();
}

}

}

extern "C"
{

G_MODULE_EXPORT
Samoyed::Plugin *createPlugin(Samoyed::PluginManager &manager,
                              const char *id,
                              GModule *module,
                              std::string &error)
{
    return new Samoyed::Terminal::TerminalPlugin(manager, id, module);
}

}
