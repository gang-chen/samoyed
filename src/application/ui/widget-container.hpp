// Widget container.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_CONTAINER_HPP
#define SMYD_WIDGET_CONTAINER_HPP

#include "widget.hpp"
#include "../utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/signals2/signal.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class WidgetContainer: public Widget
{
public:
    typedef boost::signals2::signal<void (WidgetContainer &container,
                                          Widget &child)> ChildRemoved;

    class XmlElement: public Widget::XmlElement
    {
    public:
        virtual xmlNodePtr write() const;
        XmlElement(const WidgetContainer &widget):
            Widget::XmlElement(widget)
        {}

    protected:
        XmlElement() {}
        bool readInternally(xmlNodePtr node, std::list<std::string> &errors);
    };

    virtual void grabFocus();

    virtual Widget &current();
    virtual const Widget &current() const;

    void destroyChild(Widget &child);

    virtual void removeChild(Widget &child) = 0;

    virtual void replaceChild(Widget &oldChild, Widget &newChild) = 0;

    virtual int childCount() const = 0;

    virtual Widget &child(int index) = 0;
    virtual const Widget &child(int index) const = 0;

    Widget &currentChild() { return child(currentChildIndex()); }
    const Widget &currentChild() const { return child(currentChildIndex()); }

    int childIndex(const Widget &child) const;

    virtual int currentChildIndex() const = 0;

    virtual void setCurrentChildIndex(int index) = 0;

    Widget *findChild(const char *id);
    const Widget *findChild(const char *id) const;

    virtual void onChildTitleChanged(const Widget &child) {}

    virtual void onChildDescriptionChanged(const Widget &child) {}

    boost::signals2::connection
    addChildRemovedCallback(const ChildRemoved::slot_type &callback)
    { return m_childRemoved.connect(callback); }

protected:
    WidgetContainer(): m_postponeDestroy(false), m_requestDestroy(false) {}

    void addChildInternally(Widget &child);

    void removeChildInternally(Widget &child);

    void destroyInternally();

private:
    typedef std::map<ComparablePointer<const char *>, Widget *> WidgetTable;

    WidgetTable m_childTable;

    ChildRemoved m_childRemoved;

    bool m_postponeDestroy;
    bool m_requestDestroy;
};

}

#endif
