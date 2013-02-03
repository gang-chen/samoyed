// Container of homogeneous widgets.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_CONTAINER_HPP
#define SMYD_WIDGET_CONTAINER_HPP

#include "widget.hpp"

namespace Samoyed
{

class WidgetContainer: public Widget
{
public:
    virtual Widget &current() const
    { return child(currentChildIndex()).current(); }

    virtual void addChild(Widget &child, int index) = 0;

    virtual void removeChild(Widget &child) = 0;

    /**
     * @param child The closed and deleted child.
     */
    virtual void onChildClosed(const Widget *child) = 0;

    virtual int childCount() const = 0;

    // The current child can be changed only after the widget is shown.
    virtual int currentChildIndex() const = 0;
    virtual void setCurrentChildIndex(int index) = 0;

    virtual Widget &child(int index) = 0;
    virtual const Widget &child(int index) const = 0;

    virtual int childIndex(const Widget *child) const = 0;
};

}

#endif
