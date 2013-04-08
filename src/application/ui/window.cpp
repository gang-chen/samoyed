// Top-level window.
// Copyright (C) 2011 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "window.hpp"
#include "actions.hpp"
#include "paned.hpp"
#include "widget-with-bars.hpp"
#include "notebook.hpp"
#include "../application.hpp"
#include "../utilities/miscellaneous.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <list>
#include <string>
#include <vector>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define WIDGET_CONTAINER "widget-container"
#define WINDOW "window"
#define SCREEN_INDEX "screen-index"
#define WIDTH "width"
#define HEIGHT "height"
#define IN_FULL_SCREEN "in-full-screen"
#define MAXIMIZED "maximized"
#define TOOLBAR_VISIBLE "toolbar-visible"
#define TOOLBAR_VISIBLE_IN_FULL_SCREEN "toolbar-visible-in-full-screen"
#define CHILD "child"

#define MAIN_AREA_NAME "main-area"
#define EDITOR_GROUP_NAME "editor-group"
#define PANED_NAME "paned"

#define NAVIGATION_PANE_TITLE _("_Navigation Pane")
#define TOOLS_PANE_TITLE _("_Tools Pane")

namespace
{

Samoyed::WidgetWithBars *findMainArea(Samoyed::Widget &root)
{
    if (strcmp(root.name(), MAIN_AREA_NAME) == 0)
        return static_cast<Samoyed::WidgetWithBars *>(&root);
    if (strcmp(root.name(), PANED_NAME) != 0)
        return NULL;
    Samoyed::Paned &paned = static_cast<Samoyed::Paned &>(root);
    Samoyed::WidgetWithBars *mainArea = findMainArea(paned.child(0));
    if (mainArea)
        return mainArea;
    return findMainArea(paned.child(1));
}

void setPanedPosition(GtkWidget *paned, int size)
{
    int totalSize;
    GValue handleSize = G_VALUE_INIT;
    if (gtk_orientable_get_orientation(GTK_ORIENTABLE(paned)) ==
        GTK_ORIENTATION_HORIZONTAL)
        totalSize = gtk_widget_get_allocated_width(paned);
    else
        totalSize = gtk_widget_get_allocated_height(paned);
    g_value_init(&handleSize, G_TYPE_INT);
    gtk_widget_style_get_property(paned, "handle-size", &handleSize);
    gtk_paned_set_position(GTK_PANED(paned),
                           totalSize - size - g_value_get_int(&handleSize));
    g_signal_handlers_disconnect_by_func(
        paned,
        reinterpret_cast<gpointer>(setPanedPosition),
        reinterpret_cast<gpointer>(size));
}

}

namespace Samoyed
{

const char *Window::NAME = "window";
const char *Window::NAVIGATION_PANE_NAME = "navigation-pane";
const char *Window::TOOLS_PANE_NAME = "tools-pane";

Window::Created Window::s_created;
Window::SidePaneCreated Window::s_navigationPaneCreated;
Window::SidePaneCreated Window::s_toolsPaneCreated;

bool Window::XmlElement::registerReader()
{
    return Widget::XmlElement::registerReader(WINDOW,
                                              Widget::XmlElement::Reader(read));
}

bool Window::XmlElement::readInternally(xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors)
{
    char *value, *cp;
    bool containerSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   WIDGET_CONTAINER) == 0)
        {
            if (containerSeen)
            {
                cp = g_strdup_printf(
                    _("Line %d: More than one \"%s\" elements seen.\n"),
                    child->line, WIDGET_CONTAINER);
                errors.push_back(cp);
                g_free(cp);
                return false;
            }
            if (!WidgetContainer::XmlElement::readInternally(doc,
                                                             child, errors))
                return false;
            containerSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        SCREEN_INDEX) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_configuration.m_screenIndex = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        WIDTH) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_configuration.m_width = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        HEIGHT) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_configuration.m_height = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        IN_FULL_SCREEN) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_configuration.m_inFullScreen = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        MAXIMIZED) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_configuration.m_maximized = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        TOOLBAR_VISIBLE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_configuration.m_toolbarVisible = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        TOOLBAR_VISIBLE_IN_FULL_SCREEN) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_configuration.m_toolbarVisibleInFullScreen = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CHILD) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                Widget::XmlElement *ch =
                    Widget::XmlElement::read(doc, grandChild, errors);
                if (ch)
                {
                    if (m_child)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: More than one children contained by "
                              "the bin.\n"),
                            grandChild->line);
                        errors.push_back(cp);
                        g_free(cp);
                        delete ch;
                    }
                    else
                        m_child = ch;
                }
            }
        }
    }

    if (!containerSeen)
    {
        cp = g_strdup_printf(
            _("Line %d: \"%s\" element missing.\n"),
            node->line, WIDGET_CONTAINER);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    if (!m_child)
    {
        cp = g_strdup_printf(
            _("Line %d: No child contained by the bin.\n"),
            node->line);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    return true;
}

Window::XmlElement *Window::XmlElement::read(xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &errors)
{
    XmlElement *element = new XmlElement;
    if (!element->readInternally(doc, node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr Window::XmlElement::write() const
{
    char *cp;
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(WINDOW));
    xmlAddChild(node, WidgetContainer::XmlElement::write());
    cp = g_strdup_printf("%d", m_configuration.m_screenIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(SCREEN_INDEX),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_configuration.m_width);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(WIDTH),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_configuration.m_height);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(HEIGHT),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(IN_FULL_SCREEN),
                    reinterpret_cast<const xmlChar *>(
                        m_configuration.m_inFullScreen ? "1" : "0"));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(MAXIMIZED),
                    reinterpret_cast<const xmlChar *>(
                        m_configuration.m_maximized ? "1" : "0"));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(TOOLBAR_VISIBLE),
                    reinterpret_cast<const xmlChar *>(
                        m_configuration.m_toolbarVisible ? "1" : "0"));
    xmlNewTextChild(
        node, NULL,
        reinterpret_cast<const xmlChar *>(TOOLBAR_VISIBLE_IN_FULL_SCREEN),
        reinterpret_cast<const xmlChar *>(
            m_configuration.m_toolbarVisibleInFullScreen ? "1" : "0"));
    xmlNodePtr child = xmlNewNode(NULL,
                                  reinterpret_cast<const xmlChar *>(CHILD));
    xmlAddChild(child, m_child->write());
    xmlAddChild(node, child);
    return node;
}

Window::XmlElement::XmlElement(const Window &window):
    WidgetContainer::XmlElement(window)
{
    m_configuration = window.configuration();
    m_child = window.child(0).save();
}

Widget *Window::XmlElement::restoreWidget()
{
    Window *window = new Window;
    if (!window->restore(*this))
    {
        delete window;
        return NULL;
    }
    return window;
}

Window::XmlElement::~XmlElement()
{
    delete m_child;
}

Window::Window():
    m_grid(NULL),
    m_menuBar(NULL),
    m_toolbar(NULL),
    m_child(NULL),
    m_mainArea(NULL),
    m_uiManager(NULL),
    m_actions(this),
    m_inFullScreen(false),
    m_maximized(false),
    m_toolbarVisible(true),
    m_toolbarVisibleInFullScreen(false)
{
    Application::instance().addWindow(*this);
}

bool Window::build(const Configuration &config)
{
    m_uiManager = gtk_ui_manager_new();
    gtk_ui_manager_insert_action_group(m_uiManager, m_actions.actionGroup(), 0);

    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "actions-ui.xml";
    GError *error = NULL;
    gtk_ui_manager_add_ui_from_file(m_uiManager, uiFile.c_str(), &error);
    if (error)
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to build a new window."));
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to load UI definitions from file \"%s\". %s."),
            uiFile.c_str(), error->message);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return false;
    }

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    m_grid = gtk_grid_new();

    gtk_container_add(GTK_CONTAINER(window), m_grid);

    m_menuBar = gtk_ui_manager_get_widget(m_uiManager, "/main-menu-bar");
    gtk_widget_set_hexpand(m_menuBar, TRUE);
    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            m_menuBar, NULL,
                            GTK_POS_BOTTOM, 1, 1);

    m_toolbar = gtk_ui_manager_get_widget(m_uiManager, "/main-toolbar");
    gtk_style_context_add_class(gtk_widget_get_style_context(m_toolbar),
                                GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
    gtk_widget_set_hexpand(m_toolbar, TRUE);
    GtkWidget *newPopupMenu = gtk_ui_manager_get_widget(m_uiManager,
                                                        "/new-popup-menu");
    GtkToolItem *newItem = gtk_menu_tool_button_new_from_stock(GTK_STOCK_FILE);
    gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(newItem), newPopupMenu);
    gtk_tool_item_set_tooltip_text(newItem, _("Create a file"));
    gtk_menu_tool_button_set_arrow_tooltip_text(GTK_MENU_TOOL_BUTTON(newItem),
                                                _("Create an object"));
    gtk_toolbar_insert(GTK_TOOLBAR(m_toolbar), newItem, 0);

    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            m_toolbar, m_menuBar,
                            GTK_POS_BOTTOM, 1, 1);

    g_signal_connect(window, "delete-event",
                     G_CALLBACK(onDeleteEvent), this);
    g_signal_connect(window, "focus-in-event",
                     G_CALLBACK(onFocusInEvent), this);
    g_signal_connect(window, "configure-event",
                     G_CALLBACK(onConfigureEvent), this);
    g_signal_connect(window, "window-state-event",
                     G_CALLBACK(onWindowStateEvent), this);

    setGtkWidget(window);

    // Configure the window.
    if (config.m_screenIndex >= 0)
    {
        GdkDisplay *display = gdk_display_get_default();
        GdkScreen *screen = gdk_display_get_screen(display,
                                                   config.m_screenIndex);
        gtk_window_set_screen(GTK_WINDOW(window), screen);
    }

    gtk_window_set_default_size(GTK_WINDOW(window),
                                config.m_width, config.m_height);
    m_toolbarVisible = config.m_toolbarVisible;
    m_toolbarVisibleInFullScreen = config.m_toolbarVisibleInFullScreen;
    gtk_widget_show_all(m_grid);
    if (!m_toolbarVisible)
        setToolbarVisible(false);
    if (config.m_inFullScreen)
        enterFullScreen();
    else if (config.m_maximized)
        gtk_window_maximize(GTK_WINDOW(window));

    return true;
}

bool Window::setup(const char *name, const Configuration &config)
{
    if (!WidgetContainer::setup(name))
        return false;
    if (!build(config))
        return false;

    // Create the initial editor group and the main area.
    Notebook *editorGroup =
        Notebook::create(EDITOR_GROUP_NAME, EDITOR_GROUP_NAME,
                         true, true, false);
    m_mainArea = WidgetWithBars::create(MAIN_AREA_NAME, *editorGroup);
    addChildInternally(*m_mainArea);

    s_created(*this);
    return true;
}

Window *Window::create(const char *name, const Configuration &config)
{
    Window *window = new Window;
    if (!window->setup(name, config))
    {
        delete window;
        return NULL;
    }
    gtk_widget_show(window->gtkWidget());
    return window;
}

bool Window::restore(XmlElement &xmlElement)
{
    if (!WidgetContainer::restore(xmlElement))
        return false;
    Widget *child = xmlElement.child().restoreWidget();
    if (!child)
        return false;
    if (!build(xmlElement.configuration()))
        return false;
    addChildInternally(*child);

    // Find the main area.
    m_mainArea = findMainArea(*child);
    if (!m_mainArea)
        return false;

    // Create menu items for the side panes.
    createMenuItemsForSidePanesRecursively(*child);

    gtk_widget_show(gtkWidget());
    return true;
}

Window::~Window()
{
    assert(!m_child);
    Application::instance().removeWindow(*this);
    if (m_uiManager)
        g_object_unref(m_uiManager);
}

gboolean Window::onDeleteEvent(GtkWidget *widget,
                               GdkEvent *event,
                               Window *window)
{
    if (!Application::instance().windows()->next())
    {
        // This is the last window.  Closing this window will quit the
        // application.  Confirm it.
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(window->gtkWidget()),
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

    window->close();
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

void Window::addChildInternally(Widget &child)
{
    assert(!m_child);
    WidgetContainer::addChildInternally(child);
    m_child = &child;
    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            child.gtkWidget(), m_toolbar,
                            GTK_POS_BOTTOM, 1, 1);
}

void Window::removeChildInternally(Widget &child)
{
    m_child = NULL;
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTAINER(m_grid), child.gtkWidget());
    WidgetContainer::removeChildInternally(child);
}

void Window::replaceChild(Widget &oldChild, Widget &newChild)
{
    removeChildInternally(oldChild);
    addChildInternally(newChild);
}

gboolean Window::onFocusInEvent(GtkWidget *widget,
                                GdkEvent *event,
                                Window *window)
{
    Application::instance().setCurrentWindow(*window);
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
    int totalSize;
    GValue handleSize = G_VALUE_INIT;
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
        if (gtk_widget_get_mapped(paned->gtkWidget()))
        {
            totalSize = gtk_widget_get_allocated_height(paned->gtkWidget());
            g_value_init(&handleSize, G_TYPE_INT);
            gtk_widget_style_get_property(paned->gtkWidget(),
                                          "handle-size", &handleSize);
            paned->setPosition(totalSize - size - g_value_get_int(&handleSize));
        }
        else
            g_signal_connect_after(paned->gtkWidget(),
                                   "map",
                                   G_CALLBACK(setPanedPosition),
                                   reinterpret_cast<gpointer>(size));
        break;
    case SIDE_LEFT:
        paned = Paned::split(PANED_NAME, Paned::ORIENTATION_HORIZONTAL,
                             pane, neighbor);
        paned->setPosition(size);
        break;
    case SIDE_RIGHT:
        paned = Paned::split(PANED_NAME, Paned::ORIENTATION_HORIZONTAL,
                             neighbor, pane);
        if (gtk_widget_get_mapped(paned->gtkWidget()))
        {
            totalSize = gtk_widget_get_allocated_width(paned->gtkWidget());
            g_value_init(&handleSize, G_TYPE_INT);
            gtk_widget_style_get_property(paned->gtkWidget(),
                                          "handle-size", &handleSize);
            paned->setPosition(totalSize - size - g_value_get_int(&handleSize));
        }
        else
            g_signal_connect_after(paned->gtkWidget(),
                                   "map",
                                   G_CALLBACK(setPanedPosition),
                                   reinterpret_cast<gpointer>(size));
    }

    // Add a menu item for showing or hiding the side pane.
    createMenuItemForSidePane(pane.name(), pane.title(),
                              gtk_widget_get_visible(pane.gtkWidget()));
}

Notebook &Window::navigationPane()
{
    return static_cast<Notebook &>(*findSidePane(NAVIGATION_PANE_NAME));
}

const Notebook &Window::navigationPane() const
{
    return static_cast<const Notebook &>(*findSidePane(NAVIGATION_PANE_NAME));
}

Notebook &Window::toolsPane()
{
    return static_cast<Notebook &>(*findSidePane(TOOLS_PANE_NAME));
}

const Notebook &Window::toolsPane() const
{
    return static_cast<const Notebook &>(*findSidePane(TOOLS_PANE_NAME));
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
        Notebook::create(strcmp(current.name(), EDITOR_GROUP_NAME) == 0 ?
                         EDITOR_GROUP_NAME "-2" : EDITOR_GROUP_NAME,
                         EDITOR_GROUP_NAME,
                         true, true, false);
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

void Window::createNavigationPane(Window &window)
{
    Notebook *pane =
        Notebook::create(NAVIGATION_PANE_NAME, NULL, false, false, true);
    pane->setTitle(NAVIGATION_PANE_TITLE);
    s_navigationPaneCreated(*pane);
    window.addSidePane(*pane, window.mainArea(), SIDE_LEFT, 100);
}

void Window::createToolsPane(Window &window)
{
    Notebook *pane =
        Notebook::create(TOOLS_PANE_NAME, NULL, false, false, true);
    pane->setTitle(TOOLS_PANE_TITLE);
    s_toolsPaneCreated(*pane);
    window.addSidePane(*pane, window.mainArea(), SIDE_RIGHT, 100);
}

void Window::registerDefaultSidePanes()
{
    addCreatedCallback(createNavigationPane);
    addCreatedCallback(createToolsPane);
}

void Window::createMenuItemForSidePane(const char *name, const char *title,
                                       bool visible)
{
    std::string actionName(name);
    actionName.insert(0, "show-hide-");
    std::string titleText(title);
    titleText.erase(std::remove(titleText.begin(), titleText.end(), '_'),
                    titleText.end());
    char *tooltip = g_strdup_printf(_("Show or hide %s"), titleText.c_str());
    GtkActionGroup *group = gtk_action_group_new(actionName.c_str());
    GtkToggleAction *action = gtk_toggle_action_new(actionName.c_str(),
                                                    title,
                                                    tooltip,
                                                    NULL);
    g_free(tooltip);
    gtk_toggle_action_set_active(action, visible);
    g_signal_connect(action, "toggled",
                     G_CALLBACK(showHideSidePane), this);
    gtk_action_group_add_action(group, GTK_ACTION(action));
    gtk_ui_manager_insert_action_group(m_uiManager, group, 0);
    gtk_ui_manager_add_ui(m_uiManager,
                          gtk_ui_manager_new_merge_id(m_uiManager),
                          "/main-menu-bar/view/side-panes",
                          actionName.c_str(),
                          actionName.c_str(),
                          GTK_UI_MANAGER_MENUITEM,
                          FALSE);
    g_object_unref(action);
    g_object_unref(group);
}

void Window::createMenuItemsForSidePanesRecursively(const Widget &widget)
{
    if (&widget == m_mainArea)
        return;
    if (strcmp(widget.name(), PANED_NAME) == 0)
    {
        const Paned &paned = static_cast<const Paned &>(widget);
        createMenuItemsForSidePanesRecursively(paned.child(0));
        createMenuItemsForSidePanesRecursively(paned.child(1));
        return;
    }
    createMenuItemForSidePane(widget.name(), widget.title(),
                              gtk_widget_get_visible(widget.gtkWidget()));
}

void Window::showHideSidePane(GtkToggleAction *action, Window *window)
{
    const char *name = gtk_action_get_name(GTK_ACTION(action)) + 10;
    Widget *pane = window->findSidePane(name);
    assert(pane);
    gtk_widget_set_visible(pane->gtkWidget(),
                           gtk_toggle_action_get_active(action));
}

gboolean Window::onConfigureEvent(GtkWidget *widget,
                                  GdkEvent *event,
                                  Window *window)
{
    if (!window->m_inFullScreen && !window->m_maximized)
    {
        window->m_width = event->configure.width;
        window->m_height = event->configure.height;
        printf("w %d, h %d\n", window->m_width, window->m_height);
    }
    return FALSE;
}

gboolean Window::onWindowStateEvent(GtkWidget *widget,
                                    GdkEvent *event,
                                    Window *window)
{
    window->m_maximized =
        event->window_state.new_window_state & GDK_WINDOW_STATE_MAXIMIZED;
    return FALSE;
}

void Window::setToolbarVisible(bool visible)
{
    gtk_widget_set_visible(m_toolbar, visible);
}

void Window::enterFullScreen()
{
    m_toolbarVisible = gtk_widget_get_visible(m_toolbar);
    if (m_toolbarVisible)
    {
        if (!m_toolbarVisibleInFullScreen)
            setToolbarVisible(false);
    }
    else
    {
        if (m_toolbarVisibleInFullScreen)
            setToolbarVisible(true);
    }
    gtk_widget_hide(m_menuBar);
    gtk_window_fullscreen(GTK_WINDOW(gtkWidget()));
    m_inFullScreen = true;
}

void Window::leaveFullScreen()
{
    m_toolbarVisibleInFullScreen = gtk_widget_get_visible(m_toolbar);
    if (m_toolbarVisibleInFullScreen)
    {
        if (!m_toolbarVisible)
            setToolbarVisible(false);
    }
    else
    {
        if (m_toolbarVisible)
            setToolbarVisible(true);
    }
    gtk_widget_show(m_menuBar);
    gtk_window_unfullscreen(GTK_WINDOW(gtkWidget()));
    m_inFullScreen = false;
}

Window::Configuration Window::configuration() const
{
    Configuration config;
    config.m_screenIndex =
        gdk_screen_get_number(gtk_window_get_screen(GTK_WINDOW(gtkWidget())));
    config.m_width = m_width;
    config.m_height = m_height;
    config.m_inFullScreen = m_inFullScreen;
    config.m_maximized = m_maximized;
    config.m_toolbarVisible = m_toolbarVisible;
    config.m_toolbarVisibleInFullScreen = m_toolbarVisibleInFullScreen;
    return config;
}

}
