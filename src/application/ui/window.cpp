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

#define WINDOW_ID "window"
#define MAIN_AREA_ID "main-area"
#define EDITOR_GROUP_ID "editor-group"
#define PANED_ID "paned"

namespace
{

int serialNumber = 0;

Samoyed::Widget *findPane(Samoyed::Widget &root, const char *id)
{
    if (strcmp(root.id(), id) == 0)
        return &root;
    if (strcmp(root.id(), PANED_ID) != 0)
        return NULL;
    Samoyed::Paned &paned = static_cast<Samoyed::Paned &>(root);
    Samoyed::Widget *child = findPane(paned.child(0), id);
    if (child)
        return child;
    return findPane(paned.child(1), id);
}

const Samoyed::Widget *findPane(const Samoyed::Widget &root, const char *id)
{
    if (strcmp(root.id(), id) == 0)
        return &root;
    if (strcmp(root.id(), PANED_ID) != 0)
        return NULL;
    const Samoyed::Paned &paned = static_cast<const Samoyed::Paned &>(root);
    const Samoyed::Widget *child = findPane(paned.child(0), id);
    if (child)
        return child;
    return findPane(paned.child(1), id);
}

}

namespace Samoyed
{

const char *Window::NAVIGATION_PANE_ID = "navigation-pane";
const char *Window::TOOLS_PANE_ID = "tools-pane";

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
    uiFile += G_DIR_SEPARATOR_S "ui.xml";
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
    gtk_grid_attach_next_to(GTK_GRID(m_grid),
                            m_toolbar, m_menuBar,
                            GTK_POS_BOTTOM, 1, 1);
    g_signal_connect_after(m_toolbar, "notify::visible",
                           G_CALLBACK(onToolbarVisibilityChanged), this);

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

    m_width = config.m_width;
    m_height = config.m_height;
    gtk_window_set_default_size(GTK_WINDOW(window), m_width, m_height);
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

bool Window::setup(const Configuration &config)
{
    char *id = g_strdup_printf(WINDOW_ID "-%d", serialNumber++);
    if (!WidgetContainer::setup(id))
    {
        g_free(id);
        return false;
    }
    g_free(id);
    if (!build(config))
        return false;

    // Create the initial editor group and the main area.
    Notebook *editorGroup =
        Notebook::create(EDITOR_GROUP_ID "-0", EDITOR_GROUP_ID,
                         true, true, false);
    WidgetWithBars *mainArea =
        WidgetWithBars::create(MAIN_AREA_ID, *editorGroup);
    addChildInternally(*mainArea);

    // Set the title.
    setTitle(_("Samoyed IDE"));
    gtk_window_set_title(GTK_WINDOW(gtkWidget()), title());

    s_created(*this);
    return true;
}

Window *Window::create(const Configuration &config)
{
    Window *window = new Window;
    if (!window->setup(config))
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

    // Extract the serial number of the window from its identifier, and update
    // the global serial number.
    const char *cp = strrchr(id(), '-');
    if (cp)
    {
        int sn = atoi(cp + 1);
        if (sn >= serialNumber)
            serialNumber = sn + 1;
    }

    // Create menu items for the side panes.
    createMenuItemsForSidePanesRecursively(*child);

    gtk_window_set_title(GTK_WINDOW(gtkWidget()), title());

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

void Window::removeChild(Widget &child)
{
    assert(closing());
    removeChildInternally(child);
    delete this;
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

Widget *Window::findSidePane(const char *id)
{
    return findPane(*m_child, id);
}

const Widget *Window::findSidePane(const char *id) const
{
    return findPane(*m_child, id);
}

void Window::addSidePane(Widget &pane, Widget &neighbor, Side side, double size)
{
    switch (side)
    {
    case SIDE_TOP:
        Paned::split(PANED_ID, Paned::ORIENTATION_VERTICAL,
                     pane, neighbor, size);
        break;
    case SIDE_BOTTOM:
        Paned::split(PANED_ID, Paned::ORIENTATION_VERTICAL,
                     neighbor, pane, 1. - size);
        break;
    case SIDE_LEFT:
        Paned::split(PANED_ID, Paned::ORIENTATION_HORIZONTAL,
                     pane, neighbor, size);
        break;
    default:
        Paned::split(PANED_ID, Paned::ORIENTATION_HORIZONTAL,
                     neighbor, pane, 1. - size);
    }

    // Add a menu item for showing or hiding the side pane.
    createMenuItemForSidePane(pane);
}

void Window::removeSidePane(Widget &pane)
{
    pane.parent()->removeChild(pane);
}

Notebook &Window::navigationPane()
{
    return static_cast<Notebook &>(*findSidePane(NAVIGATION_PANE_ID));
}

const Notebook &Window::navigationPane() const
{
    return static_cast<const Notebook &>(*findSidePane(NAVIGATION_PANE_ID));
}

Notebook &Window::toolsPane()
{
    return static_cast<Notebook &>(*findSidePane(TOOLS_PANE_ID));
}

const Notebook &Window::toolsPane() const
{
    return static_cast<const Notebook &>(*findSidePane(TOOLS_PANE_ID));
}

WidgetWithBars &Window::mainArea()
{
    return static_cast<WidgetWithBars &>(*findPane(*m_child, MAIN_AREA_ID));
}

const WidgetWithBars &Window::mainArea() const
{
    return
        static_cast<const WidgetWithBars &>(*findPane(*m_child, MAIN_AREA_ID));
}

Notebook &Window::currentEditorGroup()
{
    Widget *current = &mainArea().child(0).current();
    while (strncmp(current->id(), EDITOR_GROUP_ID, 12) != 0)
        current = current->parent();
    return static_cast<Notebook &>(*current);
}

const Notebook &Window::currentEditorGroup() const
{
    const Widget *current = &mainArea().child(0).current();
    while (strncmp(current->id(), EDITOR_GROUP_ID, 12) != 0)
        current = current->parent();
    return static_cast<const Notebook &>(*current);
}

Notebook *Window::splitCurrentEditorGroup(Side side)
{
    Widget &current = currentEditorGroup();
    Notebook *newEditorGroup =
        Notebook::create(*(current.id() + 13) == '0' ?
                         EDITOR_GROUP_ID "-1" : EDITOR_GROUP_ID "-0",
                         EDITOR_GROUP_ID,
                         true, true, false);
    switch (side)
    {
    case SIDE_TOP:
        Paned::split(PANED_ID, Paned::ORIENTATION_VERTICAL,
                     *newEditorGroup, current, 0.5);
        break;
    case SIDE_BOTTOM:
        Paned::split(PANED_ID, Paned::ORIENTATION_VERTICAL,
                     current, *newEditorGroup, 0.5);
        break;
    case SIDE_LEFT:
        Paned::split(PANED_ID, Paned::ORIENTATION_HORIZONTAL,
                     *newEditorGroup, current, 0.5);
        break;
    default:
        Paned::split(PANED_ID, Paned::ORIENTATION_HORIZONTAL,
                     current, *newEditorGroup, 0.5);
    }
    return newEditorGroup;
}

void Window::createNavigationPane(Window &window)
{
    Notebook *pane =
        Notebook::create(NAVIGATION_PANE_ID, NULL, false, false, true);
    pane->setTitle(_("_Navigation Pane"));
    s_navigationPaneCreated(*pane);
    window.addSidePane(*pane, window.mainArea(), SIDE_LEFT, 0.25);
}

void Window::createToolsPane(Window &window)
{
    Notebook *pane =
        Notebook::create(TOOLS_PANE_ID, NULL, false, false, true);
    pane->setTitle(_("_Tools Pane"));
    s_toolsPaneCreated(*pane);
    window.addSidePane(*pane, window.mainArea(), SIDE_RIGHT, 0.33);
}

void Window::registerDefaultSidePanes()
{
    addCreatedCallback(createNavigationPane);
    addCreatedCallback(createToolsPane);
}

void Window::createMenuItemForSidePane(Widget &pane)
{
    std::string actionName(pane.id());
    actionName.insert(0, "show-hide-");
    std::string title(pane.title());
    title.erase(std::remove(title.begin(), title.end(), '_'), title.end());
    char *tooltip = g_strdup_printf(_("Show or hide %s"), title.c_str());
    GtkToggleAction *action = gtk_toggle_action_new(actionName.c_str(),
                                                    title.c_str(),
                                                    tooltip,
                                                    NULL);
    g_free(tooltip);
    gtk_toggle_action_set_active(action,
                                 gtk_widget_get_visible(pane.gtkWidget()));
    g_signal_connect(action, "toggled",
                     G_CALLBACK(showHideSidePane), this);
    gulong signalHandlerId =
        g_signal_connect_after(pane.gtkWidget(), "notify::visible",
                               G_CALLBACK(onSidePaneVisibilityChanged), action);
    gtk_action_group_add_action(m_actions.actionGroup(), GTK_ACTION(action));
    guint mergeId = gtk_ui_manager_new_merge_id(m_uiManager);
    gtk_ui_manager_add_ui(m_uiManager,
                          mergeId,
                          "/main-menu-bar/view/side-panes",
                          actionName.c_str(),
                          actionName.c_str(),
                          GTK_UI_MANAGER_MENUITEM,
                          FALSE);
    g_object_unref(action);
    pane.addClosedCallback(
        boost::bind(&Window::onSidePaneClosed, this,
                    new SidePaneData(action, signalHandlerId, mergeId), _1));
}

void Window::createMenuItemsForSidePanesRecursively(Widget &widget)
{
    if (strcmp(widget.id(), MAIN_AREA_ID) == 0)
        return;
    if (strcmp(widget.id(), PANED_ID) == 0)
    {
        Paned &paned = static_cast<Paned &>(widget);
        createMenuItemsForSidePanesRecursively(paned.child(0));
        createMenuItemsForSidePanesRecursively(paned.child(1));
        return;
    }
    createMenuItemForSidePane(widget);
}

void Window::onSidePaneClosed(const SidePaneData *data, const Widget &pane)
{
    gtk_ui_manager_remove_ui(m_uiManager, data->m_uiMergeId);
    g_signal_handler_disconnect(pane.gtkWidget(), data->m_signalHandlerId);
    gtk_action_group_remove_action(m_actions.actionGroup(),
                                   GTK_ACTION(data->m_action));
    delete data;
}

void Window::showHideSidePane(GtkToggleAction *action, Window *window)
{
    const char *id = gtk_action_get_name(GTK_ACTION(action)) + 10;
    Widget *pane = window->findSidePane(id);
    assert(pane);
    gtk_widget_set_visible(pane->gtkWidget(),
                           gtk_toggle_action_get_active(action));
}

void Window::onSidePaneVisibilityChanged(GtkWidget *pane,
                                         GParamSpec *spec,
                                         GtkToggleAction *action)
{
    gtk_toggle_action_set_active(action, gtk_widget_get_visible(pane));
}

gboolean Window::onConfigureEvent(GtkWidget *widget,
                                  GdkEvent *event,
                                  Window *window)
{
    if (!window->m_inFullScreen && !window->m_maximized)
    {
        window->m_width = event->configure.width;
        window->m_height = event->configure.height;
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

bool Window::toolbarVisible() const
{
    return gtk_widget_get_visible(m_toolbar);
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
    m_actions.onWindowFullScreenChanged(true);
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
    m_actions.onWindowFullScreenChanged(true);
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

void Window::onToolbarVisibilityChanged(GtkWidget *toolbar,
                                        GParamSpec *spec,
                                        Window *window)
{
    window->m_actions.onToolbarVisibilityChanged(
        gtk_widget_get_visible(toolbar));
}

}
