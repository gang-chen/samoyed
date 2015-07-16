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
    class XmlElement: public WidgetContainer::XmlElement
    {
    public:
        static void registerReader();

        virtual ~XmlElement();
        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        virtual xmlNodePtr write() const;
        XmlElement(const Notebook &notebook);
        virtual Widget *restoreWidget();

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
        bool useUnderline() const { return m_useUnderline; }
        int childCount() const { return m_children.size(); }
        Widget::XmlElement &child(int index) const
        { return *m_children[index]; }
        int currentChildIndex() const { return m_currentChildIndex; }

    protected:
        XmlElement():
            m_createCloseButtons(false),
            m_canDragChildren(false),
            m_useUnderline(false),
            m_currentChildIndex(0)
        {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);

    private:
        std::string m_groupName;
        bool m_createCloseButtons;
        bool m_canDragChildren;
        bool m_useUnderline;
        std::vector<Widget::XmlElement *> m_children;
        int m_currentChildIndex;
    };

    static Notebook *create(const char *id,
                            const char *groupName,
                            bool createCloseButtons,
                            bool canDragChildren,
                            bool useUnderline);

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void destroyChild(Widget &child);

    void addChild(Widget &child, int index);

    virtual void removeChild(Widget &child);

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
    bool useUnderline() const { return m_useUnderline; }

    bool closeOnEmpty() const { return m_closeOnEmpty; }
    void setCloseOnEmpty(bool closeOnEmpty) { m_closeOnEmpty = closeOnEmpty; }

protected:
    Notebook():
        m_createCloseButtons(false),
        m_canDragChildren(false),
        m_useUnderline(false),
        m_closeOnEmpty(false)
    {}

    virtual ~Notebook();

    bool setup(const char *id, const char *groupName,
               bool createCloseButtons,
               bool canDragChildren,
               bool useUnderline);

    bool restore(XmlElement &xmlElement);

private:
    static void onCloseButtonClicked(GtkButton *button, Widget *child);

    static void onPageReordered(GtkWidget *widget, GtkWidget *child, int index,
                                Notebook *notebook);
    static void onPageAdded(GtkWidget *widget, GtkWidget *child, int index,
                            Notebook *notebook);
    static void onPageRemoved(GtkWidget *widget, GtkWidget *child, int index,
                              Notebook *notebook);

    bool m_createCloseButtons;
    bool m_canDragChildren;
    bool m_useUnderline;
    bool m_closeOnEmpty;

    std::vector<Widget *> m_children;
};

}

#endif
