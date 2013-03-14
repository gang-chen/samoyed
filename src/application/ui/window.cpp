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
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define WIDGET "widget"
#define WINDOW "window"
#define SCREEN_INDEX "screen-index"
#define X "x"
#define Y "y"
#define WIDTH "width"
#define HEIGHT "height"
#define FULL_SCREEN "full-screen"
#define MAXIMIZED "maximized"
#define TOOLBAR_VISIBLE "toolbar-visible"

#define MAIN_AREA_NAME "Main Area"
#define EDITOR_GROUP_NAME "Editor Group"
#define PANED_NAME "Paned"

#define NAVIGATION_PANE_TITLE "_Navigation Pane"
#define TOOLS_PANE_TITLE "_Tools Pane"

namespace Samoyed
{

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
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   WIDGET) == 0)
        {
            if (!WidgetContainer::XmlElement::readInternally(doc,
                                                             child, errors))
                return false;
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
                        X) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_configuration.m_x = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        Y) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_configuration.m_y = atoi(value);
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
                        FULL_SCREEN) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_configuration.m_fullScreen = atoi(value);
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

Widget::XmlElement *Window::XmlElement::read(xmlDocPtr doc,
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
    xmlNodePtr node =
        xmlNewNode(NULL, reinterpret_cast<const xmlChar *>(WINDOW));
    xmlAddChild(node, WidgetContainer::XmlElement::write());
    cp = g_strdup_printf("%d", m_configuration.m_screenIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(SCREEN_INDEX),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_configuration.m_x);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(X),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_configuration.m_y);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(Y),
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
                    reinterpret_cast<const xmlChar *>(FULL_SCREEN),
                    reinterpret_cast<const xmlChar *>(
                        m_configuration.m_fullScreen ? "1" : "0"));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(MAXIMIZED),
                    reinterpret_cast<const xmlChar *>(
                        m_configuration.m_maximized ? "1" : "0"));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(TOOLBAR_VISIBLE),
                    reinterpret_cast<const xmlChar *>(
                        m_configuration.m_toolbarVisible ? "1" : "0"));
    xmlNodePtr children =
        xmlNewNode(NULL, reinterpret_cast<const xmlChar *>(CHILD));
    xmlAddChild(children, m_child->write());
    xmlAddChild(node, children);
    return node;
}

void Window::XmlElement::savewidgetInternally(const Window &window)
{
    WidgetContainer::XmlElement::savewidgetinternally(window);
    m_configuration = window.configuration();
    m_child = window.child(0).save();
}

Window::XmlElement *Window::XmlElement::saveWidget(const Window &window)
{
    XmlElement *element = new XmlElement;
    element->saveWidgetInternally(window);
    return element;
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

bool Window::build(const Configuration &config)
{
    m_uiManager = gtk_ui_manager_new();
    gtk_ui_manager_insert_action_group(m_uiManager, m_actions.actionGroup(), 0);

    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "actions-ui.xml";
    if (gtk_ui_manager_add_ui_from_file(m_uiManager, uiFile.c_str(), NULL) == 0)
        return false;

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
    g_signal_connect(window, "window-state-event",
                     G_CALLBACK(onWindowStateEvent), this);

    setGtkWidget(window);

    // Configure the window.
    m_toolbarVisible = config.m_toolbarVisible;
    m_toolbarVisibleInFullScreen = config.m_toolbarVisibleInFullScreen;
    gtk_widget_show_all(m_grid);
    if (config.m_fullScreen)
    {
        GtkAction *viewFullScreen =
            gtk_ui_manager_get_action(m_uiManager, "/view/full-screen");
        if (viewFullScreen)
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(viewFullScreen),
                                         TRUE);
        else
            enterFullScreen();
    }
    else
    {
        if (!m_toolbarVisible)
            gtk_widget_hide(m_toolbar);
        GtkAction *viewToolbar =
            gtk_ui_manager_get_action(m_uiManager, "/view/toolbar");
        if (viewToolbar)
            gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(viewToolbar),
                                         m_toolbarVisible);
        if (config.m_maximized)
            gtk_window_maximize(window);
        else
        {
            gtk_window_move(window, config.m_x, config.m_y);
            gtk_window_resize(window, config.m_width, config.m_height);
        }
    }
    gtk_widget_show(window);

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
        Notebook::create(EDITOR_GROUP_NAME, EDITOR_GROUP_NAME, true, true);
    m_mainArea = WidgetWithBars::create(MAIN_AREA_NAME, *editorGroup);
    addChild(*m_mainArea);
}

Window *Window::create(const char *name, const Configuration &config)
{
    Window *window = new Window;
    if (!setup(name, config))
    {
        delete window;
        return NULL;
    }
    Application::instance().addWindow(*this);
    s_created(*this);
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
    addChild(*child);
    Application::instance().addWindow(*this);

    // Create menu items for the side panes.
    createMenuItemsForSidePanesRecursively(*child);

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
                               gpointer window)
{
    Window *w = static_cast<Window *>(window);

    if (!Application::instance().windows()->next())
    {
        // This is the last window.  Closing this window will quit the
        // application.  Confirm it.
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
    return Window::XmlElement::saveWidget(*this);
}

void Window::addChildInternally(Widget &child)
{
    assert(!m_child);
    WidgetContainer::addChild(child);
    m_child = &child;
    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            child.gtkWidget(), m_toolbar,
                            GTK_POS_BOTTOM, 1, 1);
}

void Window::removeChildInternally(Widget &child)
{
    m_child = NULL;
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTANDER(m_grid), child.gtkWidget());
    WidgetContainer::removeChild(child);
}

void Window::replaceChild(Widget &oldChild, Widget &newChild)
{
    removeChildInternally(oldChild);
    addChildInternally(newChild);
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

    // Add a menu item for showing or hiding the side pane.
    createMenuItemForSidePane(pane.name(), pane.title(),
                              gtk_widget_get_visible(pane.gtkWidget()));

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

void Window::createNavigationPane(Window &window)
{
    Notebook *pane = Notebook::create(NAVIGATION_PANE_NAME);
    pane->setTitle(NAVIGATION_PANE_TITLE);
    s_navigationPaneCreated(*pane);
    Configuration config = window.configuration();
    window.addSidePane(*pane, window.mainArea(), SIDE_LEFT, config.m_x / 5);
}

void Window::createToolsPane(Window &window)
{
    Notebook *pane = Notebook::create(TOOLS_PANE_NAME);
    pane->setTitle(TOOLS_PANE_TITLE);
    s_toolsPaneCreated(*pane);
    Configuration config = window.configuration();
    window.addSidePane(*pane, window.mainArea(), SIDE_RIGHT, config.m_x / 5);
}

void Window::setupDefaultSidePanes()
{
    addCreatedCallback(createNavigationPane);
    addCreatedCallback(createToolsPane);
}

void Window::createMenuItemForSidePane(const char *name, const char *title,
                                       bool visible)
{
    GtkWidget *spItem = gtk_ui_manager_get_widget(m_uiManager,
                                                  "/view/side-panes");
    if (!spItem)
        return;
    GtkWidget *item = gtk_check_menu_item_new_with_mnemonic(title);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), visible);
    char *cp = g_strdup_printf(_("Show or hide %s"), name);
    gtk_widget_set_tooltip_text(item, cp);
    g_free(cp);
    GtkWidget *spMenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(spItem));
    if (!spMenu)
    {
        spMenu = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(spItem), GTK_MENU(spMenu));
    }
    gtk_menu_shell_append(GTK_MENU_SHELL(spMenu), item);
    g_signal_connect(item, "toggled", G_CALLBACK(onSidePaneToggled), this);
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

void Window::onSidePaneToggled(GtkCheckMenuItem *menuItem, gpointer window)
{
    Window *w = static_cast<Window *>(window);
    std::string name = gtk_menu_item_get_label(GTK_MENU_ITEM(menuItem));
    std::remove(name.begin(), name.end(), '_');
    Widget *pane = w->findSidePane(name.c_str());
    assert(pane);
    gtk_widget_set_visible(pane->gtkWidget(),
                           gtk_check_menu_item_get_active(menuItem));
}

void Window::onWindowStateEvent(GtkWidget *widget,
                                GdkEvent *event,
                                gpointer window)
{
    Window *w = static_cast<Window *>(window);
    w->m_maximized = event.window_state & GDK_WINDOW_STATE_MAXIMIZED;
}

void Window::setToolbarVisible(bool visible)
{
    gtk_widget_set_visible(m_toolbar, visible);
}

void Window::setToolbarVisibleWrapper(bool visible)
{
    GtkAction *viewToolbar =
        gtk_ui_manager_get_action(m_uiManager, "/view/toolbar");
    if (viewToolbar)
        gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(viewToolbar),
                                     visible);
    else
        gtk_widget_set_visible(m_toolbar, visible);
}

void Window::enterFullScreen()
{
    m_toolbarVisible = gtk_widget_get_visible(m_toolbar);
    if (m_toolbarVisible)
    {
        if (!m_toolbarVisibleInFullScreen)
            setToolbarVisibleWrapper(false);
    }
    else
    {
        if (m_toolbarVisibleInFullScreen)
            setToolbarVisibleWrapper(true);
    }
    gtk_widget_hide(m_menuBar);
    gtk_window_fullscreen(gtkWidget());
    m_fullScreen = true;
}

void Window::leaveFullScreen()
{
    m_toolbarVisibleInFullScreen = gtk_widget_get_visible(m_toolbar);
    if (m_toolbarVisibleInFullScreen)
    {
        if (!m_toolbarVisible)
            setToolbarVisibleWrapper(false);
    }
    else
    {
        if (m_toolbarVisible)
            setToolbarVisibleWrapper(true);
    }
    gtk_widget_show(m_menuBar);
    gtk_window_unfullscreen(gtkWidget());
    m_fullScreen = false;
}

}
