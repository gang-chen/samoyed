// Extension: file browser histories.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-browser-histories-extension.hpp"
#include "application.hpp"
#include "utilities/property-tree.hpp"
#include <string>

#define FILE_BROWSER "file-browser"
#define ROOT "root"
#define VIRTUAL_ROOT "virtual-root"

namespace Samoyed
{

namespace FileBrowser
{

void FileBrowserHistoriesExtension::installHistories()
{
    PropertyTree &hist =
        Application::instance().histories().addChild(FILE_BROWSER);
    hist.addChild(ROOT, std::string());
    hist.addChild(VIRTUAL_ROOT, std::string());
}

void FileBrowserHistoriesExtension::uninstallHistories()
{
    PropertyTree &hist =
        Application::instance().histories().child(FILE_BROWSER);
    Application::instance().histories().removeChild(hist);
    delete &hist;
}

}

}
