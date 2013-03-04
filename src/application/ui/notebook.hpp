// Notebook.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_NOTEBOOK_HPP
#define SMYD_NOTEBOOK_HPP

#include "widget-container.hpp"
#include <list>
#include <string>
#include <vector>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class Notebook: public WidgetContainer
{
public:
    typedef boost::function<Widget *(const char *widgetName)> WidgetFactory;

    class XmlElement: public Widget::XmlElement
    {
    public:
        static bool registerReader();

        XmlElement(const Notebook &notebook);
        virtual ~XmlElement();
        virtual xmlNodePtr write() const;
        virtual Widget *createWidget();
        virtual bool restoreWidget(Widget &widget) const;

        /**
         * Remove a child.  When failing to create a child widget, we need to
         * remove the XML element for the widget to keep the one-to-one map
         * between the child widgets and XML elements.
         */
        void removeChild(int index);

        const char *groupName() const
        { return m_groupName.empty() ? NULL : m_groupName.c_str(); }
        bool createCloseButtons() const { return m_createCloseButtons; }
        bool canDragChildren() const { return m_canDragChildren; }
        int childCount() const { return m_children.size(); }
        Widget::XmlElement &child(int index) const
        { return *m_children[index]; }
        int currentChildIndex() const { return m_currentChildIndex; }

    protected:
        XmlElement(xmlDocPtr doc,
                   xmlNodePtr node,
                   std::list<std::string> &errors);

    private:
        static Widget::XmlElement *read(xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors);

        std::string m_groupName;
        bool m_closeButtonsExist;
        bool m_canDragChildren;
        std::vector<Widget::XmlElement *> m_children;
        int m_currentChildIndex;
    };

    Notebook(const char *name,
             const char *groupName,
             bool createCloseButtons,
             bool canDragChildren);

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    void addChild(Widget &child, int index)
    { addChildInternally(child, index); }

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    virtual int childCount() const { return m_children.size(); }

    virtual Widget &child(int index) { return *m_children[index]; }
    virtual const Widget &child(int index) const { return *m_children[index]; }

    // Assume the current child should be in the current page.
    virtual int currentChildIndex() const
    { return gtk_notebook_get_current_page(GTK_NOTEBOOK(gtkWidget())); }

    virtual void setCurrentChildIndex(int index)
    { gtk_notebook_set_current_page(GTK_NOTEBOOK(gtkWidget()), index); }

    virtual void onChildTitleChanged(const Widget &child);
    virtual void onChildDescriptionChanged(const Widget &child);

    const char *groupName() const
    { return gtk_notebook_get_group_name(GTK_NOTEBOOK(gtkWidget())); }

    bool createCloseButtons() const { return m_createCloseButtons; }
    bool canDragChildren() const { return m_canDragChildren; }

protected:
    Notebook(XmlElement &xmlElement);

    virtual ~Notebook();

    void addChildInternally(Widget &child, int index);

    virtual void removeChildInternally(Widget &child);

private:
    static void onCloseButtonClicked(GtkButton *button, gpointer child);

    static void onPageReordered(GtkWidget *widget, GtkWidget *child, int index,
                                gpointer notebook);
    static void onPageAdded(GtkWidget *widget, GtkWidget *child, int index,
                            gpointer notebook);
    static void onPageRemoved(GtkWidget *widget, GtkWidget *child, int index,
                              gpointer notebook);

    bool m_createCloseButtons;
    bool m_canDragChildren;

    std::vector<Widget *> m_children;
};

}

#endif
