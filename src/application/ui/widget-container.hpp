// Widget container.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_CONTAINER_HPP
#define SMYD_WIDGET_CONTAINER_HPP

#include "widget.hpp"

namespace Samoyed
{

class WidgetContainer: public Widget
{
public:
    /**
     * @param child The closed and deleted child.
     */
    virtual void onChildClosed(const Widget *child) = 0;

    /**
     * Replace a child with a new one.
     * @param oldChild The old child.
     * @param newChild The new child.
     */
    virtual void replaceChild(Widget &oldChild, Widget &newChild) = 0;
};

}

#endif
