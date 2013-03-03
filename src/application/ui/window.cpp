// Top-level window.
// Copyright (C) 2011 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "window.hpp"
#include "actions.hpp"
#include "widget-container.hpp"
#include "paned.hpp"
#include "widget-with-bars.hpp"
#include "notebook.hpp"
#include "project-explorer.hpp"
#include "../application.hpp"
#include "../utilities/miscellaneous.hpp"
#include <assert.h>
#include <string.h>
#include <algorithm>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#define MAIN_AREA_NAME "Main Area"
#define EDITOR_GROUP_NAME "Editor Group"
#define PANED_NAME "Paned"

namespace Samoyed
{

Window::Created Window::s_created;
Window::SidePaneCreated Window::s_navigationPaneCreated;
Window::SidePaneCreated Window::s_toolsPaneCreated;

void Window::build(const Configuration &config)
{
    m_uiManager = gtk_ui_manager_new();
    gtk_ui_manager_insert_action_group(m_uiManager, m_actions.actionGroup(), 0);

    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "actions-ui.xml";
    // Ignore the possible failure, which implies the installation is broken.
    gtk_ui_manager_add_ui_from_file(m_uiManager, uiFile.c_str(), NULL);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    m_grid = gtk_grid_new();

    gtk_container_add(GTK_CONTAINER(window), m_grid);

    m_menuBar = gtk_ui_manager_get_widget(m_uiManager, "/main-menu-bar");
    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            m_menuBar, NULL,
                            GTK_POS_BOTTOM, 1, 1);

    m_toolbar = gtk_ui_manager_get_widget(m_uiManager, "/main-toolbar");

    GtkWidget *newPopupMenu = gtk_ui_manager_get_widget(m_uiManager,
                                                        "/new-popup-menu");
    GtkToolItem *newItem = gtk_menu_tool_button_new_from_stock(GTK_STOCK_NEW);
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(newItem), newPopupMenu);
    gtk_tool_item_set_tooltip_text(newItem, _("Create an object"));
    gtk_toolbar_insert(GTK_TOOLBAR(m_toolbar), newItem, 0);

    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            m_toolbar, m_menuBar,
                            GTK_POS_BOTTOM, 1, 1);

    g_signal_connect(window, "delete-event",
                     G_CALLBACK(onDeleteEvent), this);
    g_signal_connect(window, "focus-in-event",
                     G_CALLBACK(onFocusInEvent), this);

    setGtkWidget(window);
}

Window::Window(const char *name, const Configuration &config):
    Widget(name),
    m_window(NULL),
    m_grid(NULL),
    m_menuBar(NULL),
    m_toolbar(NULL),
    m_child(NULL),
    m_uiManager(NULL),
    m_actions(this)
{
    build(config);

    // Create the initial editor group and the main area.
    Notebook *editorGroup = new Notebook(EDITOR_GROUP_NAME, EDITOR_GROUP_NAME,
                                         true, true);
    m_mainArea = new WidgetWithBars(MAIN_AREA_NAME, *editorGroup);
    addChild(*mainArea);

    Application::instance().addWindow(*this);

    s_created(*this);
}

Window::~Window()
{
    assert(!m_child);
    Application::instance().removeWindow(*this);
    if (m_uiManager)
        g_object_unref(m_uiManager);
    gtk_widget_destroy(gtkWidget());
    Application::instance().onWindowClosed();
}

gboolean Window::onDeleteEvent(GtkWidget *widget,
                               GdkEvent *event,
                               gpointer window)
{
    Window *w = static_cast<Window *>(window);

    if (&Application::instance().mainWindow() == w)
    {
        // Closing the main window will quit the application.  Confirm it.
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(w->m_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("You will quit Samoyed if you close this window. Continue "
              "closing this window and quitting Samoyed?"));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_YES);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_NO)
            return TRUE;

        // Quitting the application will destroy the window if the user doesn't
        // prevent it.
        Application::instance().quit();
        return TRUE;
    }

    w->close();
    return TRUE;
}

bool Window::close()
{
    if (closing())
        return true;

    setClosing(true);
    if (!m_child->close())
    {
        setClosing(false);
        return false;
    }
    return true;
}

Widget::XmlElement *Window::save() const
{
    return new XmlElement(*this);
}

void Window::addChild(Widget &child)
{
    assert(!child.parent());
    assert(!m_child);
    WidgetContainer::addChild(child);
    m_child = &child;
    child.setParent(this);
    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            child.gtkWidget(), m_toolbar,
                            GTK_POS_BOTTOM, 1, 1);
}

void Window::removeChild(Widget &child)
{
    assert(child.parent() == this);
    WidgetContainer::removeChild(child);
    m_child = NULL;
    child.setParent(NULL);
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTANDER(m_grid), child.gtkWidget());
}

void Window::onChildClosed(Widget *child)
{
    assert(m_child == child);
    assert(closing());
    m_child = NULL;
    delete this;
}

void Window::replaceChild(Widget &oldChild, Widget &newChild)
{
    removeChild(oldChild);
    addChild(newChild);
}

gboolean Window::onFocusInEvent(GtkWidget *widget,
                                GdkEvent *event,
                                gpointer window)
{
    Application::instance().setCurrentWindow(*static_cast<Window *>(window));
    return FALSE;
}

Widget *Window::findSidePane(const char *name)
{
    Widget *pane;
    WidgetContainer *container = m_mainArea->parent();
    while (container != this)
    {
        pane = container->findChild(name);
        if (pane)
            return pane;
        container = container->parent();
    }
    return NULL;
}

const Widget *Window::findSidePane(const char *name) const
{
    const Widget *pane;
    const WidgetContainer *container = m_mainArea->parent();
    while (container != this)
    {
        pane = container->findChild(name);
        if (pane)
            return pane;
        container = container->parent();
    }
    return NULL;
}

void Window::addSidePane(Widget &pane, Widget &neighbor, Side side, int size)
{
    Paned *paned;
    int handleSize;
    switch (side)
    {
    case SIDE_TOP:
        paned = Paned::split(PANED_NAME, Paned::ORIENTATION_VERTICAL,
                             pane, neighbor);
        paned->setPosition(size);
        break;
    case SIDE_BOTTOM:
        paned = Paned::split(PANED_NAME, Paned::ORIENTATION_VERTICAL,
                             neighbor, pane);
        gtk_widget_style_get(paned->gtkWidget(),
                             "handle-size", &handleSize,
                             NULL);
        paned->setPosition(gtk_widget_get_allocated_height(paned->gtkWidget()) -
                           size - handleSize);
        break;
    case SIDE_LEFT:
        paned = Paned::split(PANED_NAME, Paned::ORIENTATION_HORIZONTAL,
                             pane, neighbor);
        paned->setPosition(size);
        break;
    case SIDE_RIGHT:
        paned = Paned::split(PANED_NAME, Paned::ORIENTATION_HORIZONTAL,
                             neighbor, pane);
        gtk_widget_style_get(paned->gtkWidget(),
                             "handle-size", &handleSize,
                             NULL);
        paned->setPosition(gtk_widget_get_allocated_width(paned->gtkWidget()) -
                           size - handleSize);
    }
    s_sidePaneAdded(pane, *this);
}

Notebook &Window::currentEditorGroup()
{
    Widget *current = &this->current();
    while (strncmp(current->name(), EDITOR_GROUP_NAME, 12) != 0)
        current = current->parent();
    return static_cast<Notebook &>(*current);
}

const Notebook &Window::currentEditorGroup() const
{
    const Widget *current = &this->current();
    while (strncmp(current->name(), EDITOR_GROUP_NAME, 12) != 0)
        current = current->parent();
    return static_cast<const Notebook &>(*current);
}

Notebook *Window::splitCurrentEditorGroup(Side side)
{
    Widget &current = currentEditorGroup();
    Notebook *newEditorGroup =
        new Notebook(strcmp(current.name(), EDITOR_GROUP_NAME) == 0 ?
                     EDITOR_GROUP_NAME " 2" : EDITOR_GROUP_NAME,
                     EDITOR_GROUP_NAME,
                     true, true);
    switch (side)
    {
    case SIDE_TOP:
        Paned::split(PANED_NAME, Paned::ORIENTATION_VERTICAL,
                     *newEditorGroup, current);
        break;
    case SIDE_BOTTOM:
        Paned::split(PANED_NAME, Paned::ORIENTATION_VERTICAL,
                     current, *newEditorGroup);
        break;
    case SIDE_LEFT:
        Paned::split(PANED_NAME, Paned::ORIENTATION_HORIZONTAL,
                     *newEditorGroup, current);
        break;
    case SIDE_RIGHT:
        Paned::split(PANED_NAME, Paned::ORIENTATION_HORIZONTAL,
                     current, *newEditorGroup);
    }
    return newEditorGroup;
}

void createNavigationPane(Window &window)
{
    Notebook *pane = new Notebook(NAVIGATION_PANE_NAME);
    gtk_widget_set_size_request(pane->gtkWidget(),
                                WIDGET_WIDTH_REQUEST,
                                WIDGET_HEIGHT_REQUEST);
    s_navigationPaneCreated(*pane);
    Samoyed::Window::Configuration config = window.configuration();
    window.addSidePane(*pane, window.mainArea(), SIDE_LEFT, config.m_x / 5);
}

void createToolsPane(Window &window)
{
    Notebook *pane = new Notebook(TOOLS_PANE_NAME);
    gtk_widget_set_size_request(pane->gtkWidget(),
                                WIDGET_WIDTH_REQUEST,
                                WIDGET_HEIGHT_REQUEST);
    s_toolsPaneCreated(*pane);
    Samoyed::Window::Configuration config = window.configuration();
    window.addSidePane(*pane, window.mainArea(), SIDE_RIGHT, config.m_x / 5);
}

void createProjectExplorer(Widget &pane)
{
    ProjectExplorer *explorer = new ProjectExplorer(PROJECT_EXPLORER_NAME);
    gtk_widget_set_size_request(explorer->gtkWidget(),
                                WIDGET_WIDTH_REQUEST,
                                WIDGET_HEIGHT_REQUEST);
    static_cast<Notebook &>(pane).addChild(*explorer);
}

void Window::setupDefaultSidePanes()
{
    addCreatedCallback(createNavigationPane);
    addNavigationPaneCreatedCallback(createProjectExplorer);
    addCreatedCallback(createToolsPane);
}

}
