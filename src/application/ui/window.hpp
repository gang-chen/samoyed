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
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class WidgetWithBars;
class Notebook;

/**
 * A window represents a top-level window.  A window is a container that manages
 * its contents by itself.  A window contains a binary tree of panes.  The pane
 * in the center is called the main area, and the panes around it are side
 * panes that can be shown or hidden.  The main area is a widget with bars and
 * contains a binary tree of editor groups, each of which contains various
 * editors.  Generally, editors are used to edit various files, bars are used
 * to perform temporary tasks on the associated editors, and side panes are the
 * user interfaces of tools running all the time.  All side panes need to be
 * registered so that they can be managed automatically.
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

    struct PaneRecord
    {
        std::string m_name;
        WidgetFactory m_factory;
        Side m_side;
        bool m_createByDefault;
        PaneRecord(const char *name,
                   const WidgetFactory &factory,
                   Side side,
                   bool createByDefault):
            m_name(name),
            m_factory(factory),
            m_side(side),
            m_createByDefault(createByDefault)
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
     * Register an automatically managed side pane to windows with the specific
     * name.
     * @param windowWindow The name of the windows of interest, or "*" to
     * register to all windows.
     * @param paneName The name of the pane to be registered.
     * @param paneFactory The factory creating the pane.
     * @param side The side of window where the pane will be.
     * @param createByDefault True to automatically create the pane when a
     * window is created.
     */
    static void registerSidePane(const char *windowName,
                             const char *paneName,
                             const WidgetFactory &paneFactory,
                             Side sideToDock,
                             bool createByDefault);

    static void unregisterSidePane(const char *windowName,
                                   const char *paneName);

    Window(const char *name, const Configuration &config);

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

    WidgetWithBars &mainArea() { return m_mainArea; }
    const WidgetWithBars &mainArea() const { return m_mainArea; }

    Notebook &currentEditorGroup();
    const Notebook &currentEditorGroup() const;

    /**
     * Split the current editor group into two.
     * @param side The side of the current editor group where the new editor
     * group will adjoin.
     * @return The new editor group.
     */
    Notebook *splitCurrentEditorGroup(Side side);

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

    bool findSidePane(const char *name, Widget *&pane);

    void addSidePane(Widget &pane);

    static std::map<std::string, std::vector<PaneRecord> > s_sidePaneRegistry;

    static std::vector<PaneRecord> s_sidePanes;

    /**
     * The GTK+ window.
     */
    GtkWidget *m_window;

    GtkWidget *m_grid;

    GtkWidget *m_menuBar;

    GtkWidget *m_toolbar;

    Widget *m_child;

    Widget *m_mainArea;

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
