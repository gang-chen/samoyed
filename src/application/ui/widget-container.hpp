// Widget container.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_CONTAINER_HPP
#define SMYD_WIDGET_CONTAINER_HPP

#include "widget.hpp"
#include "../utilities/miscellaneous.hpp"
#include <map>

namespace Samoyed
{

class WidgetContainer: public Widget
{
public:
    WidgetContainer(const char *name): Widget(name) {}

    virtual Widget &current()
    { return child(currentChildIndex()).current(); }

    virtual const Widget &current() const
    { return child(currentChildIndex()).current(); }

    virtual void removeChild(Widget &child);

    virtual void replaceChild(Widget &oldChild, Widget &newChild) = 0;

    virtual int childCount() const = 0;

    virtual Widget &child(int index) = 0;
    virtual const Widget &child(int index) const = 0;

    int childIndex(const Widget &child) const;

    virtual int currentChildIndex() const = 0;

    virtual void setCurrentChildIndex() = 0;

    Widget *findChild(const char *name);
    const Widget *findChild(const char *name) const;

    virtual void onChildTitleChanged(const Widget &child) {}
    
    virtual void onChildDescriptionChanged(const Widget &child) {}

protected:
    void addChildInternally(Widget &child);

    virtual void removeChildInternally(Widget &child);

private:
    typedef std::map<ComparablePointer<const char *>, Widget *> WidgetTable;

    WidgetTable m_childTable;
};

}

#endif
