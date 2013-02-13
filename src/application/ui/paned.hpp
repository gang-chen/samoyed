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

    class XmlElement: public Widget::XmlElement
    {
    public:
        static bool registerReader();

        XmlElement(const Paned &paned);
        virtual ~XmlElement();
        virtual xmlNodePtr write() const;
        virtual Widget *createWidget();
        virtual bool restoreWidget(Widget &widget) const;

        Paned::Orientation orientation() const { return m_orientation; }
        Widget::XmlElement &child(int index) const
        { return *m_children[index]; }
        int position() const { return m_position; }
        int currentChildIndex() const { return m_currentChildIndex; }

    protected:
        /**
         * @throw std::runtime_error if any fatal error is found in the input
         * XML file.
         */
        XmlElement(xmlDocPtr doc,
                   xmlNodePtr node,
                   std::list<std::string> &errors);

    private:
        static Widget::XmlElement *read(xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors);

        Paned::Orientation m_orientation;
        Widget::XmlElement *m_children[2];
        int m_position;
        int m_currentChildIndex;
    };

    Paned(Orientation orientation, Widget &child1, Widget &child2);

    /**
     * @throw std::runtime_error if failing to create the paned widget.
     */
    Paned(XmlElement &xmlElement);

    virtual ~Paned();

    static Paned *split(Orientation orientation,
                        Widget &child1, Widget &child2);

    virtual GtkWidget *gtkWidget() const { return m_paned; }

    virtual Widget &current()
    { return child(currentChildIndex()).current(); }

    virtual const Widget &current() const
    { return child(currentChildIndex()).current(); }

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void onChildClosed(const Widget *child);

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    Widget &child(int index) { return *m_children[index]; }
    const Widget &child(int index) const { return *m_children[index]; }

    int childIndex(const Widget *child) const;

    int currentChildIndex() const { return m_currentChildIndex; }
    void setCurrentChildIndex(int index);

    Orientation orientation() const { return m_orientation; }

    int position() const { return gtk_paned_get_position(GTK_PANED(m_paned)); }
    void setPosition(int position)
    { gtk_paned_set_position(GTK_PANED(m_paned), position); }

protected:
    Paned(Orientation orientation);

    void addChild(Widget &child, int index);

    void removeChild(Widget &child);

private:
    GtkWidget *m_paned;

    Orientation m_orientation;

    Widget *m_children[2];

    int m_currentChildIndex;
};

}

#endif
