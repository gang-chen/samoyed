// Paned widget.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PANED_HPP
#define SMYD_PANED_HPP

#include "widget-container.hpp"
#include <list>
#include <string>
#include <gtk/gtk.h>
#include <libxml/tree.h>

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

    class XmlElement: public WidgetContainer::XmlElement
    {
    public:
        static bool registerReader();

        virtual ~XmlElement();
        static XmlElement *read(xmlDocPtr doc,
                                xmlNodePtr node,
                                std::list<std::string> &errors);
        virtual xmlNodePtr write() const;
        XmlElement(const Paned &paned);
        virtual Widget *restoreWidget();

        Paned::Orientation orientation() const { return m_orientation; }
        Widget::XmlElement &child(int index) const
        { return *m_children[index]; }
        int currentChildIndex() const { return m_currentChildIndex; }
        int position() const { return m_position; }

    protected:
        XmlElement():
            m_orientation(Paned::ORIENTATION_HORIZONTAL),
            m_currentChildIndex(0),
            m_position(-1)
        {
            m_children[0] = NULL;
            m_children[1] = NULL;
        }

        bool readInternally(xmlDocPtr doc,
                            xmlNodePtr node,
                            std::list<std::string> &errors);

    private:
        Paned::Orientation m_orientation;
        Widget::XmlElement *m_children[2];
        int m_currentChildIndex;
        int m_position;
    };

    static Paned *create(const char *name,
                         Orientation orientation,
                         Widget &child1, Widget &child2);

    static Paned *split(const char *name,
                        Orientation orientation,
                        Widget &child1, Widget &child2);

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void removeChild(Widget &child);

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    virtual int childCount() const { return 2; }

    virtual Widget &child(int index) { return *m_children[index]; }
    virtual const Widget &child(int index) const { return *m_children[index]; }

    virtual int currentChildIndex() const { return m_currentChildIndex; }
    virtual void setCurrentChildIndex(int index)
    { m_currentChildIndex = index; }

    Orientation orientation() const { return m_orientation; }

    int position() const
    { return gtk_paned_get_position(GTK_PANED(gtkWidget())); }
    void setPosition(int position)
    { gtk_paned_set_position(GTK_PANED(gtkWidget()), position); }

protected:
    Paned():
        m_orientation(ORIENTATION_HORIZONTAL),
        m_currentChildIndex(0)
    {
        m_children[0] = NULL;
        m_children[1] = NULL;
    }

    virtual ~Paned();

    bool setup(const char *name,
               Orientation orientation,
               Widget &child1, Widget &child2);

    bool restore(XmlElement &xmlElement);

    void addChildInternally(Widget &child, int index);

    void removeChildInternally(Widget &child);

private:
    static void setFocusChild(GtkWidget *container,
                              GtkWidget *child,
                              Paned *paned);

    Orientation m_orientation;

    Widget *m_children[2];

    int m_currentChildIndex;
};

}

#endif
