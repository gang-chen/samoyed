// View extension: file browser.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-browser-view-extension.hpp"
#include "file-browser-view.hpp"
#include <assert.h>

namespace Samoyed
{

namespace FileBrowser
{

View *FileBrowserViewExtension::createView(const char *viewId,
                                           const char *viewTitle)
{
    return FileBrowserView::create(viewId, viewTitle, id());
}

View *FileBrowserViewExtension::restoreView(View::XmlElement &xmlElement)
{
    // Should not happen.
    assert(0);
    return NULL;
}

void FileBrowserViewExtension::registerXmlElementReader()
{
    FileBrowserView::XmlElement::registerReader();
}

void FileBrowserViewExtension::unregisterXmlElementReader()
{
    FileBrowserView::XmlElement::unregisterReader();
}

}

}
