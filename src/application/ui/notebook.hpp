// Notebook.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_NOTEBOOK_HPP
#define SMYD_NOTEBOOK_HPP

#include "widget-container.hpp"
#include <vector>
#include <gtk/gtk.h>

namespace Samoyed
{

class Notebook: public WidgetContainer
{
public:
    class XmlElement: public Widget::XmlElement
    {
    public:
        virtual ~XmlElement();
        static XmlElement *read(xmlNodePtr node);
        virtual xmlNodePtr write() const;
        virtual Widget *restore() const;

    private:
        std::vector<Widget::XmlElement *> m_children;
        int m_currentIndex;
    };

    virtual GtkWidget *gtkWidget() const { return m_notebook; }

    virtual bool close();

    virtual XmlElement *save();

    virtual void addChild(Widget &child, int index);
    virtual void removeChild(Widget &child);

    virtual void onChildClosed(Widget *child);

    virtual int childCount() const { return m_children.size(); }

    virtual int currentChildIndex() const
    { return gtk_notebook_get_current_page(GTK_NOTEBOOK(m_notebook)); }
    virtual void setCurrentChildIndex(int index)
    { gtk_notebook_set_current_page(GTK_NOTEBOOK(m_notebook), index); }

    virtual Widget &child(int index) { return *m_children[index]; }
    virtual const Widget &child(int index) const { return *m_children[index]; }

    virtual int childIndex(const Widget *child) const;

    void onChildTitleChanged(Widget &child);

protected:
    Notebook();

    virtual ~Notebook();

private:
    static void onCloseButtonClicked(GtkButton *button, gpointer child);

    GtkWidget *m_notebook;

    std::vector<Widget *> m_children;
};

}

#endif
