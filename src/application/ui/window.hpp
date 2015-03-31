// Top-level window.
// Copyright (C) 2010 Gang Chen

#ifndef SMYD_WINDOW_HPP
#define SMYD_WINDOW_HPP

#include "widget-container.hpp"
#include "utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <boost/function.hpp>
#include <boost/signals2/signal.hpp>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class Actions;
class WidgetWithBars;
class Notebook;
class Paned;
class Editor;

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
 * A window is owned by the application instance.
 *
 * When a window is created, two side panes are created and added to the window.
 * One is the navigation pane, which is to the left of the main area.  The other
 * is the tools pane, which is to the right of the main area.
 */
class Window: public WidgetContainer
{
public:
    typedef boost::function<Widget *()> WidgetFactory;
    typedef boost::signals2::signal<void (Window &)> Created;
    typedef boost::signals2::signal<void (Window &)> Restored;
    typedef boost::signals2::signal<void (Widget &)> SidePaneCreated;

    static const char *NAVIGATION_PANE_ID;
    static const char *TOOLS_PANE_ID;

    enum Side
    {
        SIDE_TOP,
        SIDE_LEFT,
        SIDE_RIGHT,
        SIDE_BOTTOM
    };

    enum Layout
    {
        LAYOUT_TOOLS_PANE_RIGHT,
        LAYOUT_TOOLS_PANE_BOTTOM
    };

    struct Configuration
    {
        int screenIndex;
        int width;
        int height;
        bool inFullScreen;
        bool maximized;
        bool toolbarVisible;
        bool statusBarVisible;
        bool toolbarVisibleInFullScreen;
        Layout layout;
        Configuration():
            screenIndex(-1),
            width(-1),
            height(-1),
            inFullScreen(false),
            maximized(true),
            toolbarVisible(true),
            statusBarVisible(true),
            toolbarVisibleInFullScreen(false),
            layout(LAYOUT_TOOLS_PANE_RIGHT)
        {}
    };

    class XmlElement: public WidgetContainer::XmlElement
    {
    public:
        static void registerReader();

        virtual ~XmlElement();
        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        virtual xmlNodePtr write() const;
        XmlElement(const Window &window);
        virtual Widget *restoreWidget();

        const Configuration &configuration() const { return m_configuration; }
        Widget::XmlElement &child() const { return *m_child; }

    protected:
        XmlElement(): m_child(NULL) {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);

    private:
        Configuration m_configuration;
        Widget::XmlElement *m_child;
    };

    /**
     * Add a callback that will be called whenever a new window is created.
     */
    static boost::signals2::connection
    addCreatedCallback(const Created::slot_type &callback)
    { return s_created.connect(callback); }

    /**
     * Add a callback that will be called whenever a window is restored.
     */
    static boost::signals2::connection
    addRestoredCallback(const Created::slot_type &callback)
    { return s_restored.connect(callback); }

    /**
     * Add a callback that will be called whenever a navigation pane is created.
     */
    static boost::signals2::connection
    addNavigationPaneCreatedCallback(const SidePaneCreated::slot_type &callback)
    { return s_navigationPaneCreated.connect(callback); }

    /**
     * Add a callback that will be called whenever a tools pane is created.
     */
    static boost::signals2::connection
    addToolsPaneCreatedCallback(const SidePaneCreated::slot_type &callback)
    { return s_toolsPaneCreated.connect(callback); }

    static Window *create(const Configuration &config);

    /**
     * This function can be called by the application instance only.
     */
    virtual ~Window();

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void removeChild(Widget &child);

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    virtual int childCount() const { return m_child ? 1 : 0; }

    virtual Widget &child(int index) { return *m_child; }
    virtual const Widget &child(int index) const { return *m_child; }

    virtual int currentChildIndex() const { return 0; }
    virtual void setCurrentChildIndex(int index) {}

    Configuration configuration() const;

    Widget *findSidePane(const char *id);
    const Widget *findSidePane(const char *id) const;

    /**
     * Add a side pane.
     * @param pane The side pane to be added.
     * @param neighbor The widget that will be the neighbor of the side pane.
     * @param side The side the neighbor where the side pane will adjoin.
     * @param size The size of the side pane.
     */
    void addSidePane(Widget &pane, Widget &neighbor, Side side, int size);
										
    /**
     * Register a widget as a child of a side pane.  Add a menu item to let the
     * user open or close the registered widget.
     * @param paneId The identifier of the side pane to contain the widget.
     * @param id The identifier of the widget.
     * @param index The index of the widget among the children of the side pane.
     * @param factory The widget factory.
     * @param menuTitle The title of the menu item.
     */
    void registerSidePaneChild(const char *paneId, const char *id, int index,
                               const WidgetFactory &factory,
                               const char *menuTitle);

    /**
     * Unregister a child of a side pane.  If the child is open, close it
     * first.  Remove the menu item.
     * @param paneId The identifier of the side pane.
     * @param id The identifier of the child. 
     */
    void unregisterSidePaneChild(const char *paneId, const char *id);

    /**
     * Open a registered child of a side pane, if not opened.
     * @param pane The side pane.
     * @param id The identifier of the child. 
     * @return The child of the side pane.
     */
    Widget *openSidePaneChild(const char *paneId, const char *id);

    Notebook &navigationPane();
    const Notebook &navigationPane() const;
    Notebook &toolsPane();
    const Notebook &toolsPane() const;

    WidgetWithBars &mainArea();
    const WidgetWithBars &mainArea() const;

    Notebook &currentEditorGroup();
    const Notebook &currentEditorGroup() const;

    /**
     * Get the neighbor of an editor group.
     * @param side The side of the editor group where the neighbor adjoins.
     * @return The neighbor editor group.
     */
    Notebook *neighborEditorGroup(Notebook &editorGroup, Side side);
    const Notebook *neighborEditorGroup(const Notebook &editorGroup,
                                        Side side) const;

    /**
     * Split an editor group into two.
     * @param side The side of the editor group where the new editor group will
     * adjoin.
     * @return The new editor group.
     */
    Notebook *splitEditorGroup(Notebook &editorGroup, Side side);

    void addEditorToEditorGroup(Editor &editor, Notebook &editorGroup,
                                int index);

    GtkAction *addAction(const char *name,
                         const char *path,
                         const char *path2,
                         const char *label,
                         const char *tooltip,
                         const char *iconName,
                         const char *accelerator,
                         const boost::function<void (Window &,
                                                     GtkAction *)> &activate,
                         const boost::function<bool (Window &,
                                                     GtkAction *)> &sensitive,
                         bool separate);
    GtkToggleAction *addToggleAction(
        const char *name,
        const char *path,
        const char *path2,
        const char *label,
        const char *tooltip,
        const char *iconName,
        const char *accelerator,
        const boost::function<void (Window &,
                                    GtkToggleAction *)> &toggled,
        const boost::function<bool (Window &,
                                    GtkAction *)> &sensitive,
        bool activeByDefault,
        bool separate);
    void removeAction(const char *name);

    void updateActionsSensitivity();

    bool toolbarVisible() const;
    void setToolbarVisible(bool visible);

    bool statusBarVisible() const;
    void setStatusBarVisible(bool visible);

    bool inFullScreen() const { return m_inFullScreen; }
    void enterFullScreen();
    void leaveFullScreen();

    Layout layout() const;
    void changeLayout(Layout layout);

    Actions &actions() { return *m_actions; }

    static void onFileOpened(const char *uri);
    static void onFileClosed(const char *uri);

    void onCurrentFileChanged(const char *uri);

    void onCurrentTextEditorCursorChanged(int line, int column);

    static void addMessage(const char *message);
    static void removeMessage(const char *message);

protected:
    Window();

    bool setup(const Configuration &config);

    bool restore(XmlElement &xmlElement);

    void addChildInternally(Widget &child);

    void removeChildInternally(Widget &child);

    virtual void destroy();

private:
    struct SidePaneChildData
    {
        std::string id;
        int index;
        WidgetFactory factory;
        GtkToggleAction *action;
        guint uiMergeId;
        bool operator<(const SidePaneChildData &rhs) const;
    };

    struct SidePaneData
    {
        GtkToggleAction *action;
        GtkAction *action2;
        gulong signalHandlerId;
        guint uiMergeId;
        std::set<ComparablePointer<SidePaneChildData> > children;
        std::map<ComparablePointer<const char>, SidePaneChildData *> table;
    };

    struct ActionData
    {
        GtkAction *action;
        guint uiMergeId;
        guint uiMergeIdSeparator;
        guint uiMergeId2;
        guint uiMergeIdSeparator2;
        boost::function<void (GtkAction *)> activate;
        boost::function<void (GtkToggleAction *)> toggled;
        boost::function<bool (GtkAction *)> sensitive;
        ActionData():
            action(NULL),
            uiMergeId(-1),
            uiMergeIdSeparator(-1),
            uiMergeId2(-1),
            uiMergeIdSeparator2(-1)
        {}
    };

    struct FileTitleUri
    {
        char *title;
        const char *uri;
    };

    static gboolean onDeleteEvent(GtkWidget *widget,
                                  GdkEvent *event,
                                  Window *window);
    static gboolean onFocusInEvent(GtkWidget *widget,
                                   GdkEvent *event,
                                   Window *window);
    static gboolean onConfigureEvent(GtkWidget *widget,
                                     GdkEvent *event,
                                     Window *window);
    static gboolean onWindowStateEvent(GtkWidget *widget,
                                       GdkEvent *event,
                                       Window *window);
    static gboolean onKeyPressEvent(GtkWidget *widget,
                                    GdkEvent *event,
                                    Window *window);

    static void onToolbarVisibilityChanged(GtkWidget *toolbar,
                                           GParamSpec *spec,
                                           Window *window);
    static void onStatusBarVisibilityChanged(GtkWidget *statusBar,
                                             GParamSpec *spec,
                                             Window *window);

    static void showHideSidePane(GtkToggleAction *action, Window *window);
    static void onSidePaneVisibilityChanged(GtkWidget *pane,
                                            GParamSpec *spec,
                                            const SidePaneData *data);
    static void openCloseSidePaneChild(GtkToggleAction *action, Window *window);

    void createNavigationPane(Layout layout);
    void createToolsPane(Layout layout);

    bool build(const Configuration &config);

    void createMenuItemForSidePane(Widget &pane);
    void createMenuItemsForSidePanesRecursively(Widget &widget);
    void onSidePaneClosed(const Widget &pane);
    void onSidePaneChildClosed(const Widget &paneChild, const Widget &pane);

    void createStatusBar();

    void addUiForAction(const char *name,
                        const char *path,
                        guint &mergeId,
                        guint &mergeIdSeparator,
                        bool separate);

    static gboolean addMessageInMainThread(gpointer param);
    static gboolean removeMessageInMainThread(gpointer param);

    static void onCurrentFileInput(GtkComboBox *combo, Window *window);
    static void onCurrentTextEditorCursorInput(GtkEntry *entry,
                                               Window *window);

    static bool compareFileTitles(const FileTitleUri &titleUri1,
                                  const FileTitleUri &titleUri2);

    static Created s_created;
    static Restored s_restored;
    static SidePaneCreated s_navigationPaneCreated;
    static SidePaneCreated s_toolsPaneCreated;

    GtkWidget *m_menuBar;

    GtkWidget *m_toolbar;

    GtkWidget *m_statusBar;
    GtkWidget *m_currentFile;
    std::vector<FileTitleUri> m_fileTitlesUris;
    GtkWidget *m_currentLine;
    GtkWidget *m_currentColumn;
    GtkWidget *m_message;

    bool m_bypassCurrentFileChange;
    bool m_bypassCurrentFileInput;

    Widget *m_child;

    /**
     * The GTK+ UI manager that creates the menu bar, toolbar and popup menu in
     * this window.
     */
    GtkUIManager *m_uiManager;

    Actions *m_actions;

    int m_width;
    int m_height;
    bool m_inFullScreen;
    bool m_maximized;
    bool m_toolbarVisible;
    bool m_statusBarVisible;
    bool m_toolbarVisibleInFullScreen;

    std::map<ComparablePointer<const char>, SidePaneData *> m_sidePaneData;

    std::map<std::string, ActionData *> m_actionData;

    SAMOYED_DEFINE_DOUBLY_LINKED(Window)
};

}

#endif
