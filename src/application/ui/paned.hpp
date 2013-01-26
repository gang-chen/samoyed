// Paned widget.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PANED_HPP
#define SMYD_PANED_HPP

#include "widget-container.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

class Paned: public WidgetContainer
{
public:
    enum Orientation
    {
        ORIENTATION_HORIZONTAL = GTK_ORIENTATION_HORIZONTAL,
        ORIENTATION_VERTICAL = GTK_ORIENTATION_VERTICAL
    };

    class XmlElement: public Widget::XmlElement
    {
    public:
        virtual ~XmlElement();
        static XmlElement *read(xmlNodePtr node);
        virtual xmlNodePtr write() const;
        virtual Widget *restore() const;

    private:
        Orientation m_orientation;
        int m_position;
        Widget::XmlElement *m_children[2];
        int m_currentChildIndex;
    };

    static Paned *split(Orientation orientation, int position,
                        Widget &child1, Widget &child2);

    virtual GtkWidget *gtkWidget() const { return m_paned; }

    virtual bool close();

    virtual XmlElement *save();

    // A paned container needs two children.  Actually children can only be
    // replaced.
    virtual void addChild(Widget &child, int index);
    virtual void removeChild(Widget &child);

    virtual void onChildClosed(Widget *child);

    virtual int childCount() const { return 2; }

    virtual int currentChildIndex() const { return m_currentChildIndex; }
    virtual void setCurrentChildIndex(int index)
    { m_currentChildIndex = index; }

    virtual Widget &child(int index) { return *m_children[index]; }
    virtual const Widget &child(int index) const { return *m_children[index]; }

    virtual int childIndex(const Widget *child) const;

    Orientation orientation() const { return m_orientation; }

    int position() const { return gtk_paned_get_position(GTK_PANED(m_paned)); }
    void setPosition(int position)
    { gtk_paned_set_position(GTK_PANED(m_paned), position); }

protected:
    Paned(Orientation orientation, int position,
          Widget &child1, Widget &child2);

    virtual ~Paned();

private:
    GtkWidget *m_paned;

    Orientation m_orientation;

    Widget *m_children[2];

    int m_currentChildIndex;
};

}

#endif
