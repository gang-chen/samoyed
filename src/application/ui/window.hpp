// Top-level window.
// Copyright (C) 2010 Gang Chen

#ifndef SMYD_WINDOW_HPP
#define SMYD_WINDOW_HPP

#include "widget-container.hpp"
#include "actions.hpp"
#include "../utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/signals2/signal.hpp>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class WidgetWithBars;
class Notebook;
class Paned;

/**
 * A window represents a top-level window.  A window is a container that manages
 * its contents by itself.  A window contains a binary tree of panes.  The pane
 * in the center is called the main area, and the panes around it are side
 * panes that can be shown or hidden.  The main area is a widget with bars and
 * contains a binary tree of editor groups, each of which contains various
 * editors.  Generally, editors are used to edit or browse various files, bars
 * are the user interfaces of tools performing temporary tasks on the associated
 * editors, and side panes are the user interfaces of tools running all the
 * time.
 *
 * When a window is created, two side panes are created and added to the window.
 * One is the navigation pane, which is to the left of the main area.  The other
 * is the tools pane, which is to the right of the main area.
 */
class Window: public WidgetContainer
{
public:
    typedef boost::signals2::signal<void (Window &)> Created;
    typedef boost::signals2::signal<void (Widget &)> SidePaneCreated;

    static const char *NAME;
    static const char *NAVIGATION_PANE_NAME;
    static const char *TOOLS_PANE_NAME;

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
        int m_width;
        int m_height;
        bool m_inFullScreen;
        bool m_maximized;
        bool m_toolbarVisible;
        bool m_toolbarVisibleInFullScreen;
        Configuration():
            m_screenIndex(-1),
            m_width(400),
            m_height(300),
            m_inFullScreen(false),
            m_maximized(true),
            m_toolbarVisible(true),
            m_toolbarVisibleInFullScreen(false)
        {}
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
        XmlElement(const Window &window);
        virtual Widget *restoreWidget();

        const Configuration &configuration() const { return m_configuration; }
        Widget::XmlElement &child() const { return *m_child; }

    protected:
        XmlElement(): m_child(NULL) {}

        bool readInternally(xmlDocPtr doc,
                            xmlNodePtr node,
                            std::list<std::string> &errors);

    private:
        Configuration m_configuration;
        Widget::XmlElement *m_child;
    };

    /**
     * Add a callback that will be called whenever a new window is created.  The
     * callback typically creates default side panes.
     */
    static boost::signals2::connection
    addCreatedCallback(const Created::slot_type &callback)
    { return s_created.connect(callback); }

    /**
     * Add a callback that will be called whenever a navigation pane is created.
     * The callback typically creates default contents of a navigation pane.
     */
    static boost::signals2::connection
    addNavigationPaneCreatedCallback(const SidePaneCreated::slot_type &callback)
    { return s_navigationPaneCreated.connect(callback); }

    /**
     * Add a callback that will be called whenever a tools pane is created.  The
     * callback typically creates default contents of a tools pane.
     */
    static boost::signals2::connection
    addToolsPaneCreatedCallback(const SidePaneCreated::slot_type &callback)
    { return s_toolsPaneCreated.connect(callback); }

    /**
     * Register default side panes, which are the navigation pane and tools
     * pane.
     */
    static void registerDefaultSidePanes();

    static Window *create(const char *name, const Configuration &config);

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    virtual int childCount() const { return m_child ? 1 : 0; }

    virtual Widget &child(int index) { return *m_child; }
    virtual const Widget &child(int index) const { return *m_child; }

    virtual int currentChildIndex() const { return 0; }
    virtual void setCurrentChildIndex(int index) {}

    Configuration configuration() const;

    Widget *findSidePane(const char *name);
    const Widget *findSidePane(const char *name) const;

    /**
     * Add a side pane.
     * @param pane The side pane to be added.
     * @param neighbor The widget that will be the neighbor of the side pane.
     * @param side The side the neighbor where the side pane will adjoin.
     * @param size The ratio of the size of the side pane over the total size.
     */
    void addSidePane(Widget &pane, Widget &neighbor, Side side, double size);

    Notebook &navigationPane();
    const Notebook &navigationPane() const;
    Notebook &toolsPane();
    const Notebook &toolsPane() const;

    WidgetWithBars &mainArea() { return *m_mainArea; }
    const WidgetWithBars &mainArea() const { return *m_mainArea; }

    Notebook &currentEditorGroup();
    const Notebook &currentEditorGroup() const;

    /**
     * Split the current editor group into two.
     * @param side The side of the current editor group where the new editor
     * group will adjoin.
     * @return The new editor group.
     */
    Notebook *splitCurrentEditorGroup(Side side);

    void setToolbarVisible(bool visible);

    bool inFullScreen() const { return m_inFullScreen; }
    void enterFullScreen();
    void leaveFullScreen();

protected:
    Window();

    virtual ~Window();

    bool setup(const char *name, const Configuration &config);

    bool restore(XmlElement &xmlElement);

    void addChildInternally(Widget &child);

    virtual void removeChildInternally(Widget &child);

private:
    static gboolean onDeleteEvent(GtkWidget *widget,
                                  GdkEvent *event,
                                  Window *window);
    static gboolean onFocusInEvent(GtkWidget *widget,
                                   GdkEvent *event,
                                   Window *window);
    static gboolean onWindowStateEvent(GtkWidget *widget,
                                       GdkEvent *event,
                                       Window *window);
    static gboolean onConfigureEvent(GtkWidget *widget,
                                     GdkEvent *event,
                                     Window *window);
    static void showHideSidePane(GtkToggleAction *action, Window *window);

    static void createNavigationPane(Window &window);
    static void createToolsPane(Window &window);
    static void createProjectExplorer(Widget &pane);

    bool build(const Configuration &config);

    void createMenuItemForSidePane(const char *name, const char *title,
                                   bool visible);
    void createMenuItemsForSidePanesRecursively(const Widget &widget);

    static Created s_created;
    static SidePaneCreated s_navigationPaneCreated;
    static SidePaneCreated s_toolsPaneCreated;

    GtkWidget *m_grid;

    GtkWidget *m_menuBar;

    GtkWidget *m_toolbar;

    Widget *m_child;

    WidgetWithBars *m_mainArea;

    /**
     * The GTK+ UI manager that creates the menu bar, toolbar and popup menu in
     * this window.
     */
    GtkUIManager *m_uiManager;

    Actions m_actions;

    int m_width;
    int m_height;
    bool m_inFullScreen;
    bool m_maximized;
    bool m_toolbarVisible;
    bool m_toolbarVisibleInFullScreen;

    SAMOYED_DEFINE_DOUBLY_LINKED(Window)
};

}

#endif
