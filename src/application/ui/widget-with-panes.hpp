// Widget with side panes.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_WITH_PANES_HPP
#define SMYD_WIDGET_WITH_PANES_HPP

#include "widget-container.hpp"
#include <boost/function>

namespace Samoyed
{

class WidgetWithPanes: public WidgetContainer
{
public:
    typedef boost::function<Widget *(const char *containerName,
                                     const char *paneName)> paneFactory;

    enum Position
    {
        POSITION_TOP,
        POSITION_LEFT,
        POSITION_RIGHT,
        POSITION_BOTTOM
    };

    static bool registerPane(const char *containerName,
                             const char *paneName,
                             Position position,
                             const WidgetFactory &paneFactory);

    static bool unregisterPane(const char *containerName,
                               const char *paneName);
};

}

#endif
