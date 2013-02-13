// Top-level window.
// Copyright (C) 2010 Gang Chen

#ifndef SMYD_WINDOW_HPP
#define SMYD_WINDOW_HPP

#include "widget-container.hpp"
#include "actions.hpp"
#include "../utilities/misc.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

/**
 * A window represents a top-level window.
 */
class Window: public WidgetContainer
{
public:
    struct Configuration
    {
        int m_screenIndex;
        int m_x;
        int m_y;
        int m_width;
        int m_height;
        bool m_fullScreen;
        bool m_maximized;
        bool m_toolbarVisible;
        Configuration():
            m_screenIndex(-1),
            m_fullScreen(false),
            m_maximized(true),
            m_toolbarVisible(true)
        {}
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

        Widget::XmlElement &child() const { return *m_child; }

    private:
        /**
         * @throw std::runtime_error if any fatal error is found in the input
         * XML file.
         */
        XmlElement(xmlDocPtr doc,
                   xmlNodePtr node,
                   std::list<std::string> &errors);

        static Widget::XmlElement *read(xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors);

        Widget::XmlElement *m_child;
    };

    Window(const Configuration &config, Widget &child);

    virtual GtkWidget *gtkWidget() const { return m_window; }

    virtual Widget &current() { return m_child->current(); }
    virtual const Widget &current() const { return m_child->current(); }

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void onChildClosed(const Widget *child);

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    Widget &child() { return *m_child; }
    const Widget &child() const { return *m_child; }

    Configuration configuration() const;

private:
    static gboolean onDeleteEvent(GtkWidget *widget,
                                  GdkEvent *event,
                                  gpointer window);
    static void onDestroy(GtkWidget *widget, gpointer window);
    static gboolean onFocusInEvent(GtkWidget *widget,
                                   GdkEvent *event,
                                   gpointer window);

    virtual ~Window();

    void addChild(Widget &child);

    void removeChild(Widget &child);

    /**
     * The GTK+ window.
     */
    GtkWidget *m_window;

    GtkWidget *m_grid;

    GtkWidget *m_menuBar;

    GtkWidget *m_toolbar;

    /**
     * The GTK+ UI manager that creates the menu bar, toolbar and popup menu in
     * this window.
     */
    GtkUIManager *m_uiManager;

    Actions m_actions;

    Widget *m_child;

    SAMOYED_DEFINE_DOUBLY_LINKED(Window)
};

}

#endif
