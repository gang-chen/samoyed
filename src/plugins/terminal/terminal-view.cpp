// View: terminal.
// Copyright (C) 2013 Gang Chen.

#include "terminal-view.hpp"
#include <vte/vte.h>

namespace Samoyed
{

bool TerminalView::setup(const char *id, const char *title)
{
    if (!View::setup(id))
        return false;
    setTitle(title);
    GtkWidget *term = vte_terminal_new();
    setGtkWidget(term);
    gtk_widget_show_all(term);
    return true;
}

bool TerminalView::restore(XmlElement &xmlElement)
{
    if (!View::restore(xmlElement))
        return false;
    GtkWidget *term = vte_terminal_new();
    setGtkWidget(term);
    if (xmlElement.visible())
        gtk_widget_show_all(term);
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

}
