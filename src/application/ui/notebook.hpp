// Notebook.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_NOTEBOOK_HPP
#define SMYD_NOTEBOOK_HPP

#include "widget-container.hpp"
#include <list>
#include <map>
#include <string>
#include <vector>
#include <boost/function.hpp>
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

        int childCount() const { return m_children.size(); }
        Widget::XmlElement &child(int index) const
        { return *m_children[index]; }
        int currentChildIndex() const { return m_currentChildIndex; }
        bool closeButtonExists() const { return m_closeButtonExists; }

    protected:
        XmlElement(xmlDocPtr doc,
                   xmlNodePtr node,
                   std::list<std::string> &errors);

    private:
        static Widget::XmlElement *read(xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors);

        std::vector<Widget::XmlElement *> m_children;
        int m_currentChildIndex;
        bool m_closeButtonsExist;
    };

    /**
     * Register a default child to notebooks with the specific name.  If a
     * notebook with the name is created, the registered default children will
     * be automatically created and added to the notebook.
     * @param notebookName The name of the notebooks of interest, or "*" to
     * register to all notebooks.
     * @param childName The name of the default child to be registered.
     * @param childFactory The factory creating the default child.
     */
    static void registerDefaultChild(const char *notebookName,
                                     const char *childName,
                                     const WidgetFactory &childFactory);

    static void unregisterDefaultChild(const char *notebookName,
                                       const char *childName);

    Notebook(const char *name, bool createCloseButton);

    virtual GtkWidget *gtkWidget() const { return m_notebook; }

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void onChildClosed(const Widget *child);

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    void addChild(Widget &child, int index);

    void removeChild(Widget &child);

    virtual int childCount() const { return m_children.size(); }

    virtual Widget &child(int index) { return *m_children[index]; }
    virtual const Widget &child(int index) const { return *m_children[index]; }

    // Assume the current child should be in the current page.
    virtual int currentChildIndex() const
    { return gtk_notebook_get_current_page(GTK_NOTEBOOK(m_notebook)); }

    virtual void setCurrentChildIndex(int index)
    { gtk_notebook_set_current_page(GTK_NOTEBOOK(m_notebook), index); }

    virtual void onChildTitleChanged(const Widget &child);
    virtual void onChildDescriptionChanged(const Widget &child);

protected:
    Notebook(XmlElement &xmlElement);

    virtual ~Notebook();

private:
    static void onCloseButtonClicked(GtkButton *button, gpointer child);

    static std::map<std::string, std::map<std::string, WidgetFactory> >
        s_defaultChildRegistry;

    static std::map<std::string, WidgetFactory> s_defaultChildRegistryForAll;

    GtkWidget *m_notebook;

    std::vector<Widget *> m_children;

    bool m_createCloseButtons;
};

}

#endif
