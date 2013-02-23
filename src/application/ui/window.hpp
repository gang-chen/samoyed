// Top-level window.
// Copyright (C) 2010 Gang Chen

#ifndef SMYD_WINDOW_HPP
#define SMYD_WINDOW_HPP

#include "widget-container.hpp"
#include "actions.hpp"
#include "../utilities/misc.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/function.hpp>
#include <boost/tuple/tuple.hpp>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

/**
 * A window represents a top-level window.
 */
class Window: public WidgetContainer
{
public:
    typedef boost::function<Widget *(widgetName)> WidgetFactory;

    enum Side
    {
        SIDE_TOP,
        SIDE_LEFT,
        SIDE_RIGHT,
        SIDE_BOTTOM
    };

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

    /**
     * Register an automatically managed pane to windows with the specific name.
     * @param windowWindow The name of the windows of interest, or "*" to
     * register to all windows.
     * @param paneName The name of the pane to be registered.
     * @param side The side of window where the pane will be.
     * @param create True to automatically create the pane when a window is
     * created.
     * @param paneFactory The factory creating the pane.
     */
    static void registerPane(const char *windowName,
                             const char *paneName,
                             Side side,
                             bool create,
                             const WidgetFactory &paneFactory);

    static void unregisterPane(const char *windowName, const char *paneName);

    /**
     * @param child The child widget, which will be the widget in the center
     * and an indirect child if some side panes are created.
     */
    Window(const char *name, const Configuration &config, Widget &child);

    virtual GtkWidget *gtkWidget() const { return m_window; }

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void onChildClosed(const Widget *child);

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    virtual int childCount() const { return 1; }

    virtual Widget &child(int index) { return *m_child; }
    virtual const Widget &child(int index) const { return *m_child; }

    virtual int currentChildIndex() const { return 0; }
    virtual void setCurrentChildIndex(int index) {}

    Configuration configuration() const;

    void split(Widget &child, Side side);

private:
    static gboolean onDeleteEvent(GtkWidget *widget,
                                  GdkEvent *event,
                                  gpointer window);
    static void onDestroy(GtkWidget *widget, gpointer window);
    static gboolean onFocusInEvent(GtkWidget *widget,
                                   GdkEvent *event,
                                   gpointer window);

    Window(XmlElement &xmlElement);

    virtual ~Window();

    void initialize(const Configuration &config);

    void addChild(Widget &child);

    void removeChild(Widget &child);

    bool findPane(const char *name, Widget &widget);

    static std::map<std::string,
                    std::map<std::string,
                             boost::tuple<Side, bool, WidgetFactory> > >
        s_managedPaneRegistry;

    static std::map<std::string, boost::tuple<Side, bool, WidgetFactory> >
        s_managedPanes;

    /**
     * The GTK+ window.
     */
    GtkWidget *m_window;

    GtkWidget *m_grid;

    GtkWidget *m_menuBar;

    GtkWidget *m_toolbar;

    Widget *m_child;

    /**
     * The GTK+ UI manager that creates the menu bar, toolbar and popup menu in
     * this window.
     */
    GtkUIManager *m_uiManager;

    Actions m_actions;

    SAMOYED_DEFINE_DOUBLY_LINKED(Window)
};

}

#endif
