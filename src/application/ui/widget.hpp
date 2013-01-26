// Widget.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_HPP
#define SMYD_WIDGET_HPP

#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class WidgetContainer;

class Widget
{
public:
    class XmlElement
    {
    public:
        virtual ~XmlElement() {}
        virtual xmlNodePtr write() const = 0;
        virtual Widget *restore() const = 0;
    };

    /**
     * @return The underlying GTK+ widget.  Note that it is read-only.
     */
    virtual GtkWidget *gtkWidget() const = 0;

    virtual const char *title() const { return ""; }

    virtual const char *description() const { return ""; }

    virtual Widget &current() const { return *this; }

    WidgetContainer *parent() const { return m_parent; }

    void setParent(WidgetContainer *parent) { m_parent = parent; }

    bool closing() const { return m_closing; }

    virtual bool close() = 0;

    virtual XmlElement *save() = 0;

protected:
    Widget(): m_parent(NULL), m_closing(false) {}

    virtual ~Widget();

    void setClosing(bool closing) { m_closing = closing; }

private:
    WidgetContainer *m_parent;

    bool m_closing;
};

}

#endif
