// View extension: file browser.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-browser-view-extension.hpp"
#include "file-browser-view.hpp"
#include "file-browser-plugin.hpp"
#include <boost/bind.hpp>
#include <boost/ref.hpp>

namespace Samoyed
{

namespace FileBrowser
{

View *FileBrowserViewExtension::createView(const char *viewId,
                                           const char *viewTitle)
{
    FileBrowserView *view = FileBrowserView::create(viewId, viewTitle, id());
    if (!view)
        return NULL;
    FileBrowserPlugin::instance().onViewCreated(*view);
    view->addClosedCallback(boost::bind(
        &FileBrowserPlugin::onViewClosed,
        boost::ref(FileBrowserPlugin::instance()),
        _1));
    return view;
}

View *FileBrowserViewExtension::restoreView(View::XmlElement &xmlElement)
{
    FileBrowserView *view = FileBrowserView::restore(xmlElement, id());
    if (!view)
        return NULL;
    FileBrowserPlugin::instance().onViewCreated(*view);
    view->addClosedCallback(boost::bind(
        &FileBrowserPlugin::onViewClosed,
        boost::ref(FileBrowserPlugin::instance()),
        _1));
    return view;
}

}

}
