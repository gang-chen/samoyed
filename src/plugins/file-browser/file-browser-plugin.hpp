// Plugin: file browser.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FLBR_FILE_BROWSER_PLUGIN_HPP
#define SMYD_FLBR_FILE_BROWSER_PLUGIN_HPP

#include "utilities/plugin.hpp"
#include <set>

namespace Samoyed
{

class Widget;

namespace FileBrowser
{

class FileBrowserPlugin: public Plugin
{
public:
    static FileBrowserPlugin &instance() { return *s_instance; }

    FileBrowserPlugin(PluginManager &manager, const char *id, GModule *module);

    virtual void deactivate();

    void onViewCreated(Widget &view);
    void onViewClosed(Widget &view);

protected:
    virtual Extension *createExtension(const char *extensionId);

    virtual bool completed() const;

private:
    static FileBrowserPlugin *s_instance;

    std::set<Widget *> m_views;
};

}

}

#endif
