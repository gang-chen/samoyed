// View: terminal.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "terminal-view.hpp"
#include "terminal.hpp"

namespace Samoyed
{

namespace Terminal
{

TerminalView::TerminalView(const char *extensionId):
    View(extensionId),
    m_terminal(new Terminal(*this))
{
}

TerminalView::~TerminalView()
{
    m_terminal->destroy();
}

bool TerminalView::setupTerminal()
{
    GtkWidget *widget = m_terminal->setup();
    if (!widget)
        return false;
    setGtkWidget(widget);
    return true;
}

bool TerminalView::setup(const char *id, const char *title)
{
    if (!View::setup(id))
        return false;
    setTitle(title);
    if (!setupTerminal())
        return false;
    gtk_widget_show_all(gtkWidget());
    return true;
}

bool TerminalView::restore(XmlElement &xmlElement)
{
    if (!View::restore(xmlElement))
        return false;
    if (!setupTerminal())
        return false;
    if (xmlElement.visible())
        gtk_widget_show_all(gtkWidget());
    return true;
}

TerminalView *TerminalView::create(const char *id, const char *title,
                                   const char *extensionId)
{
    TerminalView *view = new TerminalView(extensionId);
    if (!view->setup(id, title))
    {
        delete view;
        return NULL;
    }
    return view;
}

TerminalView *TerminalView::restore(XmlElement &xmlElement,
                                    const char *extensionId)
{
    TerminalView *view = new TerminalView(extensionId);
    if (!view->restore(xmlElement))
    {
        delete view;
        return NULL;
    }
    return view;
}

void TerminalView::grabFocus()
{
    m_terminal->grabFocus();
}

}

}
