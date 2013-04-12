// Widget container.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_CONTAINER_HPP
#define SMYD_WIDGET_CONTAINER_HPP

#include "widget.hpp"
#include "../utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

class WidgetContainer: public Widget
{
public:
    class XmlElement: public Widget::XmlElement
    {
    public:
        virtual xmlNodePtr write() const;
        XmlElement(const WidgetContainer &widget):
            Widget::XmlElement(widget)
        {}

    protected:
        XmlElement() {}
        bool readInternally(xmlDocPtr doc,
                            xmlNodePtr node,
                            std::list<std::string> &errors);
    };

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

    virtual void setCurrentChildIndex(int index) = 0;

    Widget *findChild(const char *id);
    const Widget *findChild(const char *id) const;

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
