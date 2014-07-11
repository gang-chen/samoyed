// Action extension: find text.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "find-text-action-extension.hpp"
#include "text-finder-bar.hpp"
#include "finder-plugin.hpp"
#include "ui/window.hpp"
#include "ui/widget-with-bars.hpp"
#include <boost/bind.hpp>
#include <boost/ref.hpp>

namespace Samoyed
{

namespace Finder
{

void FindTextActionExtension::activateAction(Window &window, GtkAction *action)
{
    TextFinderBar *bar = TextFinderBar::create();
    window.mainArea().addBar(*bar);
    FinderPlugin::instance().onTextFinderBarCreated(*bar);
    bar->addClosedCallback(boost::bind(&FinderPlugin::onTextFinderBarClosed,
                                       boost::ref(FinderPlugin::instance()),
                                       _1));
}

bool FindTextActionExtension::isActionSensitive(Window &window,
                                                GtkAction *action)
{
    return true;
}

}

}

#endif
