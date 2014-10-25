// View: file browser.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-browser-view.hpp"
#include "../../../libs/gedit-file-browser/gedit-file-browser-widget.h"

namespace Samoyed
{

namespace FileBrowser
{

bool FileBrowserView::setup(const char *id, const char *title)
{
    if (!View::setup(id))
        return false;
    setTitle(title);
    GtkWidget *widget = gedit_file_browser_widget_new();
    if (!widget)
        return false;
    setGtkWidget(widget);
    gtk_widget_show_all(widget);
    return true;
}

bool FileBrowserView::restore(XmlElement &xmlElement)
{
    if (!View::restore(xmlElement))
        return false;
    GtkWidget *widget = gedit_file_browser_widget_new();
    if (!widget)
        return false;
    setGtkWidget(widget);
    if (xmlElement.visible())
        gtk_widget_show_all(widget);
    return true;
}

FileBrowserView *FileBrowserView::create(const char *id, const char *title,
                                         const char *extensionId)
{
    FileBrowserView *view = new FileBrowserView(extensionId);
    if (!view->setup(id, title))
    {
        delete view;
        return NULL;
    }
    return view;
}

FileBrowserView *FileBrowserView::restore(XmlElement &xmlElement,
                                          const char *extensionId)
{
    FileBrowserView *view = new FileBrowserView(extensionId);
    if (!view->restore(xmlElement))
    {
        delete view;
        return NULL;
    }
    return view;
}

}

}
