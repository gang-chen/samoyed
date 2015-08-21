// Plugin: file browser.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-browser-plugin.hpp"
#include "file-browser-view-extension.hpp"
#include "widget/widget.hpp"
#include <string.h>

namespace Samoyed
{

namespace FileBrowser
{

FileBrowserPlugin *FileBrowserPlugin::s_instance = NULL;

FileBrowserPlugin::FileBrowserPlugin(PluginManager &manager,
                                     const char *id,
                                     GModule *module):
    Plugin(manager, id, module)
{
    s_instance = this;
}

Extension *FileBrowserPlugin::createExtension(const char *extensionId)
{
    if (strcmp(extensionId, "file-browser/view") == 0)
        return new FileBrowserViewExtension(extensionId, *this);
    return NULL;
}

void FileBrowserPlugin::onViewCreated(Widget &view)
{
    m_views.insert(&view);
}

void FileBrowserPlugin::onViewClosed(Widget &view)
{
    m_views.erase(&view);
    if (completed())
        onCompleted();
}

bool FileBrowserPlugin::completed() const
{
    return m_views.empty();
}

void FileBrowserPlugin::deactivate()
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
    return new Samoyed::FileBrowser::FileBrowserPlugin(manager, id, module);
}

}
