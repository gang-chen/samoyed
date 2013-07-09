// View extension: terminal.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "terminal-view-extension.hpp"
#include "terminal-view.hpp"
#include "terminal-plugin.hpp"
#include <boost/bind.hpp>
#include <boost/ref.hpp>

namespace Samoyed
{

View *TerminalViewExtension::createView(const char *viewId,
                                        const char *viewTitle)
{
    TerminalView *view = TerminalView::create(viewId, viewTitle, id());
    TerminalPlugin &plugin = static_cast<TerminalPlugin &>(m_plugin);
    plugin.onViewCreated();
    // XXX The callback doesn't take any argument.
    view->addClosedCallback(boost::bind(&TerminalPlugin::onViewClosed,
                                        boost::ref(plugin)));
    return view;
}

View *TerminalViewExtension::restoreView(View::XmlElement &xmlElement)
{
    TerminalView *view = TerminalView::restore(xmlElement, id());
    TerminalPlugin &plugin = static_cast<TerminalPlugin &>(m_plugin);
    plugin.onViewCreated();
    // XXX The callback doesn't take any argument.
    view->addClosedCallback(boost::bind(&TerminalPlugin::onViewClosed,
                                        boost::ref(plugin)));
    return view;
}

}
