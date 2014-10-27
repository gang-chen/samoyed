// View: file browser.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_FLBR_FILE_BROWSER_VIEW_HPP
#define SMYD_FLBR_FILE_BROWSER_VIEW_HPP

#include "ui/view.hpp"

namespace Samoyed
{

namespace FileBrowser
{

class FileBrowserView: public View
{
public:
    static FileBrowserView *create(const char *id, const char *title,
                                   const char *extensionId);

    static FileBrowserView *restore(XmlElement &xmlElement,
                                    const char *extensionId);

protected:
    FileBrowserView(const char *extensionId): View(extensionId) {}

    bool setupFileBrowser();

    bool setup(const char *id, const char *title);

    bool restore(XmlElement &xmlElement);
};

}

}

#endif
