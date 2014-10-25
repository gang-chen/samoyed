// View extension: file browser.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FLBR_FILE_BROWSER_VIEW_EXTENSION_HPP
#define SMYD_FLBR_FILE_BROWSER_VIEW_EXTENSION_HPP

#include "ui/view-extension.hpp"

namespace Samoyed
{

namespace FileBrowser
{

class FileBrowserViewExtension: public ViewExtension
{
public:
    FileBrowserViewExtension(const char *id, Plugin &plugin):
        ViewExtension(id, plugin)
    {}

    virtual View *createView(const char *viewId, const char *viewTitle);
    virtual View *restoreView(View::XmlElement &xmlElement);
};

}

}

#endif
