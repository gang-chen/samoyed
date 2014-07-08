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
#include "editor.hpp"
#include "text-editor.hpp"
#include "application.hpp"
#include "utilities/miscellaneous.hpp"
#include "utilities/worker.hpp"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <list>
#include <string>
#include <vector>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#include <glib/gi18n.h>
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
#define STATUS_BAR_VISIBLE "status-bar-visible"
#define TOOLBAR_VISIBLE_IN_FULL_SCREEN "toolbar-visible-in-full-screen"
#define CHILD "child"

#define WINDOW_ID "window"
#define MAIN_AREA_ID "main-area"
#define EDITOR_GROUP_ID "editor-group"
#define PANED_ID "paned"

#define SIDE_PANE_MENU_ITEM_TITLE "side-pane-menu-item-title"
#define SIDE_PANE_CHILDREN_MENU_TITLE "side-pane-children-menu-title"
#define SIDE_PANE_CHILD_MENU_ITEM_TITLE "side-pane-child-manu-item-title"

namespace
{

const double DEFAULT_SIZE_RATIO = 0.7;

const double SIDE_PANE_SIZE_RATIO = 0.25;

const int STATUS_BAR_MARGIN = 6;

const int LINE_NUMBER_WIDTH = 6;

const int COLUMN_NUMBER_WIDTH = 4;

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

void onEditorRemoved(Samoyed::WidgetContainer &container,
                     Samoyed::Widget &child)
{
    // If this is the last editor in the group and the group is not the last
    // group in the window, close the group.
    if (container.childCount() == 0 &&
        strcmp(container.parent()->id(), MAIN_AREA_ID) != 0)
        static_cast<Samoyed::Notebook &>(container).setAutomaticClose(true);
}

void addEditorRemovedCallbacks(Samoyed::Widget &widget)
{
    if (strcmp(widget.id(), PANED_ID) == 0)
    {
        Samoyed::Paned &paned = static_cast<Samoyed::Paned &>(widget);
        addEditorRemovedCallbacks(paned.child(0));
        addEditorRemovedCallbacks(paned.child(1));
        return;
    }
    static_cast<Samoyed::WidgetContainer &>(widget).
        addChildRemovedCallback(onEditorRemoved);
}

void activateAction(GtkAction *action,
                    boost::function<void (GtkAction *)> *activate)
{
    (*activate)(action);
}

void onActionToggled(GtkToggleAction *action,
                     boost::function<void (GtkToggleAction *)> *toggled)
{
    (*toggled)(action);
}

}

namespace Samoyed
{

const char *Window::NAVIGATION_PANE_ID = "navigation-pane";
const char *Window::TOOLS_PANE_ID = "tools-pane";

Window::Created Window::s_created;
Window::Created Window::s_restored;
Window::SidePaneCreated Window::s_navigationPaneCreated;
Window::SidePaneCreated Window::s_toolsPaneCreated;

void Window::XmlElement::registerReader()
{
    Widget::XmlElement::registerReader(WINDOW,
                                       Widget::XmlElement::Reader(read));
}

bool Window::XmlElement::readInternally(xmlNodePtr node,
                                        std::list<std::string> &errors)
{
    char *value, *cp;
    bool containerSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
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
            if (!WidgetContainer::XmlElement::readInternally(child, errors))
                return false;
            containerSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        SCREEN_INDEX) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_configuration.m_screenIndex =
                        boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid integer \"%s\" for element \"%s\". "
                          "%s.\n"),
                        child->line, value, SCREEN_INDEX, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        WIDTH) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_configuration.m_width = boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid integer \"%s\" for element \"%s\". "
                          "%s.\n"),
                        child->line, value, WIDTH, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        HEIGHT) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_configuration.m_height = boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid integer \"%s\" for element \"%s\". "
                          "%s.\n"),
                        child->line, value, HEIGHT, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        IN_FULL_SCREEN) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_configuration.m_inFullScreen =
                        boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid Boolean value \"%s\" for element "
                          "\"%s\". %s.\n"),
                        child->line, value, IN_FULL_SCREEN, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        MAXIMIZED) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_configuration.m_maximized =
                        boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid Boolean value \"%s\" for element "
                          "\"%s\". %s.\n"),
                        child->line, value, MAXIMIZED, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        TOOLBAR_VISIBLE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_configuration.m_toolbarVisible =
                        boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid Boolean value \"%s\" for element "
                          "\"%s\". %s.\n"),
                        child->line, value, TOOLBAR_VISIBLE, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        STATUS_BAR_VISIBLE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_configuration.m_statusBarVisible =
                        boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid Boolean value \"%s\" for element "
                          "\"%s\". %s.\n"),
                        child->line, value, STATUS_BAR_VISIBLE, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        TOOLBAR_VISIBLE_IN_FULL_SCREEN) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_configuration.m_toolbarVisibleInFullScreen =
                        boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid Boolean value \"%s\" for element "
                          "\"%s\". %s.\n"),
                        child->line, value, TOOLBAR_VISIBLE_IN_FULL_SCREEN,
                        exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CHILD) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                if (grandChild->type != XML_ELEMENT_NODE)
                    continue;
                Widget::XmlElement *ch =
                    Widget::XmlElement::read(grandChild, errors);
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

Window::XmlElement *Window::XmlElement::read(xmlNodePtr node,
                                             std::list<std::string> &errors)
{
    XmlElement *element = new XmlElement;
    if (!element->readInternally(node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr Window::XmlElement::write() const
{
    std::string str;
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(WINDOW));
    xmlAddChild(node, WidgetContainer::XmlElement::write());
    str = boost::lexical_cast<std::string>(m_configuration.m_screenIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(SCREEN_INDEX),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.m_width);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(WIDTH),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.m_height);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(HEIGHT),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.m_inFullScreen);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(IN_FULL_SCREEN),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.m_maximized);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(MAXIMIZED),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.m_toolbarVisible);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(TOOLBAR_VISIBLE),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.m_statusBarVisible);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(STATUS_BAR_VISIBLE),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(
        m_configuration.m_toolbarVisibleInFullScreen);
    xmlNewTextChild(
        node, NULL,
        reinterpret_cast<const xmlChar *>(TOOLBAR_VISIBLE_IN_FULL_SCREEN),
        reinterpret_cast<const xmlChar *>(str.c_str()));
    xmlNodePtr child = xmlNewNode(NULL,
                                  reinterpret_cast<const xmlChar *>(CHILD));
    xmlAddChild(child, m_child->write());
    xmlAddChild(node, child);
    return node;
}

Window::XmlElement::XmlElement(const Window &window):
    WidgetContainer::XmlElement(window),
    m_configuration(window.configuration())
{
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
    m_menuBar(NULL),
    m_toolbar(NULL),
    m_statusBar(NULL),
    m_currentFile(NULL),
    m_currentLine(NULL),
    m_currentColumn(NULL),
    m_workerCount(numberOfProcessors()),
    m_workersStatus(NULL),
    m_child(NULL),
    m_uiManager(NULL),
    m_actions(new Actions(this)),
    m_inFullScreen(false),
    m_maximized(false),
    m_toolbarVisible(true),
    m_statusBarVisible(true),
    m_toolbarVisibleInFullScreen(false)
{
    Application::instance().addWindow(*this);
}

bool Window::build(const Configuration &config)
{
    m_uiManager = gtk_ui_manager_new();
    gtk_ui_manager_insert_action_group(m_uiManager,
                                       m_actions->actionGroup(),
                                       0);

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
    gtk_window_add_accel_group(GTK_WINDOW(window),
                               gtk_ui_manager_get_accel_group(m_uiManager));

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    m_menuBar = gtk_ui_manager_get_widget(m_uiManager, "/main-menu-bar");
    gtk_widget_set_hexpand(m_menuBar, TRUE);
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            m_menuBar, NULL,
                            GTK_POS_BOTTOM, 1, 1);

    m_toolbar = gtk_ui_manager_get_widget(m_uiManager, "/main-toolbar");
    gtk_style_context_add_class(gtk_widget_get_style_context(m_toolbar),
                                GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
    gtk_widget_set_hexpand(m_toolbar, TRUE);
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            m_toolbar, m_menuBar,
                            GTK_POS_BOTTOM, 1, 1);
    g_signal_connect_after(m_toolbar, "notify::visible",
                           G_CALLBACK(onToolbarVisibilityChanged), this);

    createStatusBar();
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            m_statusBar, m_toolbar,
                            GTK_POS_BOTTOM, 1, 1);

    g_signal_connect(window, "delete-event",
                     G_CALLBACK(onDeleteEvent), this);
    g_signal_connect(window, "focus-in-event",
                     G_CALLBACK(onFocusInEvent), this);
    g_signal_connect(window, "configure-event",
                     G_CALLBACK(onConfigureEvent), this);
    g_signal_connect(window, "window-state-event",
                     G_CALLBACK(onWindowStateEvent), this);
    g_signal_connect(window, "key-press-event",
                     G_CALLBACK(onKeyPressEvent), this);

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
    if (m_width == -1)
    {
        GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
        m_width = gdk_screen_get_width(screen) * DEFAULT_SIZE_RATIO;
        m_height = gdk_screen_get_height(screen) * DEFAULT_SIZE_RATIO;
    }
    gtk_window_set_default_size(GTK_WINDOW(window), m_width, m_height);

    m_toolbarVisible = config.m_toolbarVisible;
    m_statusBarVisible = config.m_statusBarVisible;
    m_toolbarVisibleInFullScreen = config.m_toolbarVisibleInFullScreen;
    gtk_widget_show_all(grid);
    if (!m_toolbarVisible)
        setToolbarVisible(false);
    if (!m_statusBarVisible)
        setStatusBarVisible(false);
    if (config.m_inFullScreen)
        enterFullScreen();
    else if (config.m_maximized)
        gtk_window_maximize(GTK_WINDOW(window));

    return true;
}

bool Window::setup(const Configuration &config)
{
    std::string id(WINDOW_ID "-");
    id += boost::lexical_cast<std::string>(serialNumber++);
    if (!WidgetContainer::setup(id.c_str()))
        return false;
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
    window->grabFocus();
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
        try
        {
            int sn = boost::lexical_cast<int>(cp + 1);
            if (sn >= serialNumber)
                serialNumber = sn + 1;
        }
        catch (boost::bad_lexical_cast &)
        {
        }
    }

    // Start to observe all editors.
    addEditorRemovedCallbacks(mainArea().mainChild());

    // Setup the side panes.
    setupSidePanesRecursively(*child);

    gtk_window_set_title(GTK_WINDOW(gtkWidget()), title());

    s_restored(*this);

    grabFocus();
    gtk_widget_show(gtkWidget());
    return true;
}

Window::~Window()
{
    assert(!m_child);
    delete[] m_workersStatus;
    Application::instance().removeWindow(*this);
    if (m_uiManager)
        g_object_unref(m_uiManager);
}

void Window::destroy()
{
    Application::instance().destroyWindow(*this);
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
    gtk_grid_attach_next_to(GTK_GRID(gtk_bin_get_child(GTK_BIN(gtkWidget()))),
                            child.gtkWidget(), m_statusBar,
                            GTK_POS_BOTTOM, 1, 1);
}

void Window::removeChildInternally(Widget &child)
{
    assert(&child == m_child);
    m_child = NULL;
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTAINER(gtk_bin_get_child(GTK_BIN(gtkWidget()))),
                         child.gtkWidget());
    WidgetContainer::removeChildInternally(child);
}

void Window::removeChild(Widget &child)
{
    assert(closing());
    removeChildInternally(child);
    destroyInternally();
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

void Window::addSidePane(Widget &pane, Widget &neighbor, Side side, int size)
{
    switch (side)
    {
    case SIDE_TOP:
        Paned::split(PANED_ID, Paned::ORIENTATION_VERTICAL, 0,
                     pane, neighbor, size);
        break;
    case SIDE_BOTTOM:
        Paned::split(PANED_ID, Paned::ORIENTATION_VERTICAL, 1,
                     neighbor, pane, size);
        break;
    case SIDE_LEFT:
        Paned::split(PANED_ID, Paned::ORIENTATION_HORIZONTAL, 0,
                     pane, neighbor, size);
        break;
    default:
        Paned::split(PANED_ID, Paned::ORIENTATION_HORIZONTAL, 1,
                     neighbor, pane, size);
    }

    // Add a menu item for showing or hiding the side pane.
    createMenuItemForSidePane(pane);
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

Notebook *Window::neighborEditorGroup(Notebook &editorGroup, Side side)
{
    for (Widget *child = &editorGroup, *parent = child->parent();
         strcmp(parent->id(), PANED_ID) == 0;
         child = parent, parent = parent->parent())
    {
        Paned *paned = static_cast<Paned *>(parent);
        switch (side)
        {
        case SIDE_TOP:
            if (paned->orientation() == Paned::ORIENTATION_VERTICAL &&
                &paned->child(1) == child)
                return static_cast<Notebook *>(&paned->child(0));
            break;
        case SIDE_BOTTOM:
            if (paned->orientation() == Paned::ORIENTATION_VERTICAL &&
                &paned->child(0) == child)
                return static_cast<Notebook *>(&paned->child(1));
            break;
        case SIDE_LEFT:
            if (paned->orientation() == Paned::ORIENTATION_HORIZONTAL &&
                &paned->child(1) == child)
                return static_cast<Notebook *>(&paned->child(0));
            break;
        default:
            if (paned->orientation() == Paned::ORIENTATION_HORIZONTAL &&
                &paned->child(0) == child)
                return static_cast<Notebook *>(&paned->child(1));
        }
    }
    return NULL;
}

const Notebook *Window::neighborEditorGroup(const Notebook &editorGroup,
                                            Side side) const
{
    for (const Widget *child = &editorGroup, *parent = child->parent();
         strcmp(parent->id(), PANED_ID) == 0;
         child = parent, parent = parent->parent())
    {
        const Paned *paned = static_cast<const Paned *>(parent);
        switch (side)
        {
        case SIDE_TOP:
            if (paned->orientation() == Paned::ORIENTATION_VERTICAL &&
                &paned->child(1) == child)
                return static_cast<const Notebook *>(&paned->child(0));
            break;
        case SIDE_BOTTOM:
            if (paned->orientation() == Paned::ORIENTATION_VERTICAL &&
                &paned->child(0) == child)
                return static_cast<const Notebook *>(&paned->child(1));
            break;
        case SIDE_LEFT:
            if (paned->orientation() == Paned::ORIENTATION_HORIZONTAL &&
                &paned->child(1) == child)
                return static_cast<const Notebook *>(&paned->child(0));
            break;
        default:
            if (paned->orientation() == Paned::ORIENTATION_HORIZONTAL &&
                &paned->child(0) == child)
                return static_cast<const Notebook *>(&paned->child(1));
        }
    }
    return NULL;
}

Notebook *Window::splitEditorGroup(Notebook &editorGroup, Side side)
{
    Notebook *newEditorGroup =
        Notebook::create(*(editorGroup.id() + 13) == '0' ?
                         EDITOR_GROUP_ID "-1" : EDITOR_GROUP_ID "-0",
                         EDITOR_GROUP_ID,
                         true, true, false);
    switch (side)
    {
    case SIDE_TOP:
        Paned::split(PANED_ID, Paned::ORIENTATION_VERTICAL,
                     *newEditorGroup, editorGroup, 0.5);
        break;
    case SIDE_BOTTOM:
        Paned::split(PANED_ID, Paned::ORIENTATION_VERTICAL,
                     editorGroup, *newEditorGroup, 0.5);
        break;
    case SIDE_LEFT:
        Paned::split(PANED_ID, Paned::ORIENTATION_HORIZONTAL,
                     *newEditorGroup, editorGroup, 0.5);
        break;
    default:
        Paned::split(PANED_ID, Paned::ORIENTATION_HORIZONTAL,
                     editorGroup, *newEditorGroup, 0.5);
    }
    return newEditorGroup;
}

void Window::addEditorToEditorGroup(Editor &editor, Notebook &editorGroup,
                                    int index)
{
    editorGroup.addChild(editor, index);
    editorGroup.addChildRemovedCallback(onEditorRemoved);
}

void Window::createNavigationPane(Window &window)
{
    Notebook *pane =
        Notebook::create(NAVIGATION_PANE_ID, NULL, false, false, true);
    pane->setTitle(_("_Navigation Pane"));
    pane->setProperty(SIDE_PANE_MENU_ITEM_TITLE, _("_Navigation Pane"));
    pane->setProperty(SIDE_PANE_CHILDREN_MENU_TITLE, _("Na_vigators"));
    window.addSidePane(*pane, window.mainArea(), SIDE_LEFT,
                       window.configuration().m_width * SIDE_PANE_SIZE_RATIO);
    s_navigationPaneCreated(*pane);
}

void Window::createToolsPane(Window &window)
{
    Notebook *pane =
        Notebook::create(TOOLS_PANE_ID, NULL, false, false, true);
    pane->setTitle(_("_Tools Pane"));
    pane->setProperty(SIDE_PANE_MENU_ITEM_TITLE, _("_Tools Pane"));
    pane->setProperty(SIDE_PANE_CHILDREN_MENU_TITLE, _("T_ools"));
    window.addSidePane(*pane, window.mainArea(), SIDE_RIGHT,
                       window.configuration().m_width * SIDE_PANE_SIZE_RATIO);
    s_toolsPaneCreated(*pane);
}

void Window::registerDefaultSidePanes()
{
    addCreatedCallback(createNavigationPane);
    addCreatedCallback(createToolsPane);
}

void Window::createMenuItemForSidePane(Widget &pane)
{
    SidePaneData *data = new SidePaneData;

    std::string actionName("show-hide-");
    actionName += pane.id();

    const std::string *menuTitle = pane.getProperty(SIDE_PANE_MENU_ITEM_TITLE);
    std::string str;
    if (!menuTitle)
    {
        // To be safe, remove all underscores.
        str = pane.title();
        str.erase(std::remove(str.begin(), str.end(), '_'), str.end());
        menuTitle = &str;
    }

    std::string menuTooltip(pane.title());
    menuTooltip.erase(std::remove(menuTooltip.begin(), menuTooltip.end(), '_'),
                      menuTooltip.end());
    std::transform(menuTooltip.begin(), menuTooltip.end(), menuTooltip.begin(),
                  tolower);
    menuTooltip.insert(0, _("Show or hide "));

    GtkToggleAction *action = gtk_toggle_action_new(actionName.c_str(),
                                                    menuTitle->c_str(),
                                                    menuTooltip.c_str(),
                                                    NULL);
    gtk_toggle_action_set_active(action,
                                 gtk_widget_get_visible(pane.gtkWidget()));
    g_signal_connect(action, "toggled",
                     G_CALLBACK(showHideSidePane), this);
    gtk_action_group_add_action(m_actions->actionGroup(), GTK_ACTION(action));
    g_object_unref(action);

    gulong signalHandlerId =
        g_signal_connect_after(pane.gtkWidget(), "notify::visible",
                               G_CALLBACK(onSidePaneVisibilityChanged), data);

    guint mergeId = gtk_ui_manager_new_merge_id(m_uiManager);
    gtk_ui_manager_add_ui(m_uiManager,
                          mergeId,
                          "/main-menu-bar/view/side-panes",
                          actionName.c_str(),
                          actionName.c_str(),
                          GTK_UI_MANAGER_MENUITEM,
                          FALSE);

    std::string actionName2(pane.id());
    actionName2 += "-children";

    const std::string *menuTitle2 =
        pane.getProperty(SIDE_PANE_CHILDREN_MENU_TITLE);
    if (!menuTitle2)
    {
        // To be safe, remove all underscores.
        str = pane.title();
        str.erase(std::remove(str.begin(), str.end(), '_'), str.end());
        str += _(" Children");
        menuTitle2 = &str;
    }

    GtkAction *action2 = gtk_action_new(actionName2.c_str(),
                                        menuTitle2->c_str(),
                                        NULL,
                                        NULL);
    gtk_action_group_add_action(m_actions->actionGroup(), action2);
    g_object_unref(action2);

    gtk_ui_manager_add_ui(m_uiManager,
                          mergeId,
                          "/main-menu-bar/view/side-panes",
                          actionName2.c_str(),
                          actionName2.c_str(),
                          GTK_UI_MANAGER_MENU,
                          FALSE);

    std::string uiPath("/main-menu-bar/view/side-panes/");
    uiPath += actionName2;
    std::string placeHolder("placeholder1");
    for (int i = 1; i < 10; ++i, ++*placeHolder.rbegin())
        gtk_ui_manager_add_ui(m_uiManager,
                              mergeId,
                              uiPath.c_str(),
                              placeHolder.c_str(),
                              NULL,
                              GTK_UI_MANAGER_PLACEHOLDER,
                              FALSE);
    gtk_ui_manager_add_ui(m_uiManager,
                          mergeId,
                          uiPath.c_str(),
                          "placeholder-ext",
                          NULL,
                          GTK_UI_MANAGER_PLACEHOLDER,
                          FALSE);

    pane.addClosedCallback(boost::bind(&Window::onSidePaneClosed, this, _1));

    data->action = action;
    data->action2 = action2;
    data->signalHandlerId = signalHandlerId;
    data->uiMergeId = mergeId;

    m_sidePaneData.insert(std::make_pair(pane.id(), data));
}

void Window::setupSidePanesRecursively(Widget &widget)
{
    if (strcmp(widget.id(), MAIN_AREA_ID) == 0)
        return;
    if (strcmp(widget.id(), PANED_ID) == 0)
    {
        Paned &paned = static_cast<Paned &>(widget);
        setupSidePanesRecursively(paned.child(0));
        setupSidePanesRecursively(paned.child(1));
        return;
    }

    createMenuItemForSidePane(widget);
}

void Window::onSidePaneClosed(const Widget &pane)
{
    SidePaneData *data = m_sidePaneData[pane.id()];
    gtk_ui_manager_remove_ui(m_uiManager, data->uiMergeId);
    g_signal_handler_disconnect(pane.gtkWidget(), data->signalHandlerId);
    gtk_action_group_remove_action(m_actions->actionGroup(),
                                   GTK_ACTION(data->action));
    gtk_action_group_remove_action(m_actions->actionGroup(),
                                   GTK_ACTION(data->action2));
    m_sidePaneData.erase(pane.id());
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
                                         const SidePaneData *data)
{
    gtk_toggle_action_set_active(data->action, gtk_widget_get_visible(pane));
    gtk_action_set_visible(data->action2, gtk_widget_get_visible(pane));
}

void Window::registerSidePaneChild(const char *paneId,
                                   const char *id, int index,
                                   const WidgetFactory &factory,
                                   const char *menuTitle)
{
    const WidgetContainer *pane =
        static_cast<const WidgetContainer *>(findSidePane(paneId));
    assert(pane);

    SidePaneChildData *data = new SidePaneChildData;

    data->id = id;
    data->index = index;
    data->factory = factory;

    std::string actionName("open-close-");
    actionName += paneId;
    actionName += '+';
    actionName += id;

    std::string menuTooltip(menuTitle);
    menuTooltip.erase(std::remove(menuTooltip.begin(), menuTooltip.end(), '_'),
                      menuTooltip.end());
    std::transform(menuTooltip.begin(), menuTooltip.end(), menuTooltip.begin(),
                  tolower);
    menuTooltip.insert(0, _("Open or close "));

    GtkToggleAction *action = gtk_toggle_action_new(actionName.c_str(),
                                                    menuTitle,
                                                    menuTooltip.c_str(),
                                                    NULL);
    gtk_toggle_action_set_active(action, pane->findChild(id) ? TRUE : FALSE);
    g_signal_connect(action, "toggled",
                     G_CALLBACK(openCloseSidePaneChild), this);
    gtk_action_group_add_action(m_actions->actionGroup(), GTK_ACTION(action));
    g_object_unref(action);

    std::string uiPath("/main-menu-bar/view/side-panes/");
    uiPath += paneId;
    uiPath += "-children/";
    if (index > 9)
        uiPath += "placeholder-ext";
    else
    {
        uiPath += "placeholder1";
        *uiPath.rbegin() += index - 1;
    }
    guint mergeId = gtk_ui_manager_new_merge_id(m_uiManager);
    gtk_ui_manager_add_ui(m_uiManager,
                          mergeId,
                          uiPath.c_str(),
                          actionName.c_str(),
                          actionName.c_str(),
                          GTK_UI_MANAGER_MENUITEM,
                          FALSE);
    data->action = action;
    data->uiMergeId = mergeId;

    SidePaneData *paneData = m_sidePaneData[paneId];
    paneData->children.insert(data);
    paneData->table.insert(std::make_pair(data->id.c_str(), data));
}

void Window::unregisterSidePaneChild(const char *paneId, const char *id)
{
    // If the child is opened, close it.
    WidgetContainer *pane =
        static_cast<WidgetContainer *>(findSidePane(paneId));
    assert(pane);
    Widget *child = pane->findChild(id);
    // Assume the child will be closed immediately.
    if (child)
        child->close();

    SidePaneData *paneData = m_sidePaneData[paneId];
    SidePaneChildData *data = paneData->table[id];
    gtk_ui_manager_remove_ui(m_uiManager, data->uiMergeId);
    gtk_action_group_remove_action(m_actions->actionGroup(),
                                   GTK_ACTION(data->action));
    paneData->children.erase(data);
    paneData->table.erase(id);
    delete data;
}

Widget *Window::openSidePaneChild(const char *paneId, const char *id)
{
    Notebook *pane = static_cast<Notebook*>(findSidePane(paneId));
    assert(pane);
    Widget *child = pane->findChild(id);
    if (child)
        return child;

    SidePaneData *paneData = m_sidePaneData[paneId];
    SidePaneChildData *data = paneData->table[id];
    child = data->factory();
    if (child)
    {
        int index = 0;
        std::set<ComparablePointer<SidePaneChildData> >::const_iterator end =
            paneData->children.find(data);
        for (std::set<ComparablePointer<SidePaneChildData> >::const_iterator
             it = paneData->children.begin();
             it != end;
             ++it)
            if ((*it)->id == pane->child(index).id())
                ++index;
        pane->addChild(*child, index);
        child->setCurrent();
        child->addClosedCallback(boost::bind(&Window::onSidePaneChildClosed,
                                             this, _1, boost::cref(*pane)));
        gtk_toggle_action_set_active(data->action, TRUE);
    }
    else
        gtk_toggle_action_set_active(data->action, FALSE);
    return child;
}

void Window::onSidePaneChildClosed(const Widget &paneChild, const Widget &pane)
{
    SidePaneData *paneData = m_sidePaneData[pane.id()];
    SidePaneChildData *data = paneData->table[paneChild.id()];
    gtk_toggle_action_set_active(data->action, FALSE);
}

void Window::openCloseSidePaneChild(GtkToggleAction *action, Window *window)
{
    char *paneId = strdup(gtk_action_get_name(GTK_ACTION(action)) + 11);
    char *childId = strchr(paneId, '+');
    assert(childId);
    *childId = '\0';
    ++childId;
    WidgetContainer *pane =
        static_cast<WidgetContainer *>(window->findSidePane(paneId));
    assert(pane);
    Widget *child = pane->findChild(childId);
    if (gtk_toggle_action_get_active(action))
    {
        if (!child)
            window->openSidePaneChild(paneId, childId);
    }
    else
    {
        // Assume the child will be closed.
        if (child)
            child->close();
    }
    free(paneId);
}

bool Window::SidePaneChildData::operator<(const SidePaneChildData &rhs) const
{
    if (index < rhs.index)
        return true;
    if (rhs.index < index)
        return false;
    return id < rhs.id;
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

bool Window::statusBarVisible() const
{
    return gtk_widget_get_visible(m_statusBar);
}

void Window::setStatusBarVisible(bool visible)
{
    gtk_widget_set_visible(m_statusBar, visible);
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
    gtk_widget_hide(m_statusBar);
    gtk_window_fullscreen(GTK_WINDOW(gtkWidget()));
    m_inFullScreen = true;
    m_actions->onWindowFullScreenChanged(true);
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
    gtk_widget_show(m_statusBar);
    gtk_window_unfullscreen(GTK_WINDOW(gtkWidget()));
    m_inFullScreen = false;
    m_actions->onWindowFullScreenChanged(false);
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
    config.m_statusBarVisible = m_statusBarVisible;
    config.m_toolbarVisibleInFullScreen = m_toolbarVisibleInFullScreen;
    return config;
}

void Window::onToolbarVisibilityChanged(GtkWidget *toolbar,
                                        GParamSpec *spec,
                                        Window *window)
{
    window->m_actions->onToolbarVisibilityChanged(
        gtk_widget_get_visible(toolbar));
}

void Window::onStatusBarVisibilityChanged(GtkWidget *statusBar,
                                          GParamSpec *spec,
                                          Window *window)
{
    window->m_actions->onStatusBarVisibilityChanged(
        gtk_widget_get_visible(statusBar));
}

gboolean Window::onKeyPressEvent(GtkWidget *widget,
                                 GdkEvent *event,
                                 Window *window)
{
    gboolean handled = FALSE;

    // Handle focus widget key events.
    if (!handled)
        handled = gtk_window_propagate_key_event(GTK_WINDOW(widget),
                                                 &event->key);

    // Handle mnemonics and accelerators.
    if (!handled)
        handled = gtk_window_activate_key(GTK_WINDOW(widget),
                                          &event->key);

    // The above code is copied from the default handler of GtkWindow but the
    // order is swapped.  If not handled, the default handler will execute the
    // code again but won't take any action.  This is a waste of time.  But I
    // don't know how to bypass the code.
    return handled;
}

GtkAction *Window::addAction(
    const char *actionName,
    const char *actionPath,
    const char *menuTitle,
    const char *menuTooltip,
    const char *accelerator,
    const boost::function<void (Window &, GtkAction *)> &activate,
    const boost::function<bool (Window &, GtkAction *)> &sensitive)
{
    ActionData *data = new ActionData;
    data->activate = boost::bind(activate, boost::ref(*this), _1);
    data->sensitive = boost::bind(sensitive, boost::ref(*this), _1);

    GtkAction *action =
        gtk_action_new(actionName, menuTitle, menuTooltip, NULL);
    g_signal_connect(action, "activate",
                     G_CALLBACK(activateAction), &data->activate);
    gtk_action_group_add_action_with_accel(m_actions->actionGroup(),
                                           action,
                                           accelerator);
    g_object_unref(action);

    guint uiMergeId = gtk_ui_manager_new_merge_id(m_uiManager);
    gtk_ui_manager_add_ui(m_uiManager,
                          uiMergeId,
                          actionPath,
                          actionName,
                          actionName,
                          GTK_UI_MANAGER_MENUITEM,
                          FALSE);
    data->action = action;
    data->uiMergeId = uiMergeId;
    m_actionData[actionName] = data;
    return action;
}

GtkToggleAction *Window::addToggleAction(
    const char *actionName,
    const char *actionPath,
    const char *menuTitle,
    const char *menuTooltip,
    const char *accelerator,
    const boost::function<void (Window &, GtkToggleAction *)> &toggled,
    const boost::function<bool (Window &, GtkAction *)> &sensitive,
    bool activeByDefault)
{
    ActionData *data = new ActionData;
    data->toggled = boost::bind(toggled, boost::ref(*this), _1);
    data->sensitive = boost::bind(sensitive, boost::ref(*this), _1);

    GtkToggleAction *action =
        gtk_toggle_action_new(actionName, menuTitle, menuTooltip, NULL);
    gtk_toggle_action_set_active(action, activeByDefault);
    g_signal_connect(action, "toggled",
                     G_CALLBACK(onActionToggled), &data->toggled);
    gtk_action_group_add_action_with_accel(m_actions->actionGroup(),
                                           GTK_ACTION(action),
                                           accelerator);
    g_object_unref(action);

    guint uiMergeId = gtk_ui_manager_new_merge_id(m_uiManager);
    gtk_ui_manager_add_ui(m_uiManager,
                          uiMergeId,
                          actionPath,
                          actionName,
                          actionName,
                          GTK_UI_MANAGER_MENUITEM,
                          FALSE);
    data->action = GTK_ACTION(action);
    data->uiMergeId = uiMergeId;
    m_actionData[actionName] = data;
    return action;
}

void Window::removeAction(const char *actionName)
{
    ActionData *data = m_actionData[actionName];
    gtk_ui_manager_remove_ui(m_uiManager, data->uiMergeId);
    gtk_action_group_remove_action(m_actions->actionGroup(), data->action);
    m_actionData.erase(actionName);
    delete data;
}

void Window::updateActionsSensitivity()
{
    for (std::map<std::string, ActionData *>::iterator it =
         m_actionData.begin();
         it != m_actionData.end();
         ++it)
        gtk_action_set_sensitive(it->second->action,
                                 it->second->sensitive(it->second->action));
}

void Window::createStatusBar()
{
    m_statusBar = gtk_grid_new();
    GtkWidget *label;
    label = gtk_label_new(_("File:"));
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            label, NULL,
                            GTK_POS_RIGHT, 1, 1);
    m_currentFile = gtk_combo_box_text_new();

    // Add already opened files.
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        char *fileName = g_filename_from_uri(file->uri(), NULL, NULL);
        char *title = g_filename_display_basename(fileName);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(m_currentFile),
                                       title);
        g_free(title);
        g_free(fileName);
    }

    gtk_widget_set_tooltip_text(
        m_currentFile,
        _("Select which file to edit"));
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            m_currentFile, label,
                            GTK_POS_RIGHT, 1, 1);
    label = gtk_label_new(_("Line:"));
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            label, m_currentFile,
                            GTK_POS_RIGHT, 1, 1);
    m_currentLine = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(m_currentLine),
                              LINE_NUMBER_WIDTH);
    gtk_widget_set_tooltip_text(
        m_currentLine,
        _("Input the line number to go"));
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            m_currentLine, label,
                            GTK_POS_RIGHT, 1, 1);
    label = gtk_label_new(_("Column:"));
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            label, m_currentLine,
                            GTK_POS_RIGHT, 1, 1);
    m_currentColumn = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(m_currentColumn),
                              COLUMN_NUMBER_WIDTH);
    gtk_widget_set_tooltip_text(
        m_currentColumn,
        _("Input the column number to go"));
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            m_currentColumn, label,
                            GTK_POS_RIGHT, 1, 1);

    label = gtk_label_new(_("Background workers:"));
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            label, m_currentColumn,
                            GTK_POS_RIGHT, 1, 1);
    m_workersStatus = new GtkWidget *[m_workerCount];
    for (int i = 0; i < m_workerCount; ++i)
    {
        m_workersStatus[i] = gtk_spinner_new();
        gtk_widget_set_tooltip_text(m_workersStatus[i], _("Idle"));
        gtk_grid_attach_next_to(
            GTK_GRID(m_statusBar),
            m_workersStatus[i],
            i == 0 ? label : m_workersStatus[i - 1],
            GTK_POS_RIGHT, 1, 1);
    }

    gtk_grid_set_column_spacing(GTK_GRID(m_statusBar), CONTAINER_SPACING);
    gtk_widget_set_margin_left(m_statusBar, STATUS_BAR_MARGIN);
    gtk_widget_set_margin_right(m_statusBar, STATUS_BAR_MARGIN);

    gtk_widget_set_hexpand(m_statusBar, TRUE);
    g_signal_connect_after(m_statusBar, "notify::visible",
                           G_CALLBACK(onStatusBarVisibilityChanged), this);
}

void Window::onFileOpened(const char *uri)
{
    char *fileName = g_filename_from_uri(uri, NULL, NULL);
    char *title = g_filename_display_basename(fileName);
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
    {
        // The window has not been setup completely in the early stage of
        // restoring a session.
        if (window->m_currentFile)
            gtk_combo_box_text_append_text(
                GTK_COMBO_BOX_TEXT(window->m_currentFile),
                title);
    }
    g_free(title);
    g_free(fileName);
}

void Window::onFileClosed(const char *uri)
{
    char *fileName = g_filename_from_uri(uri, NULL, NULL);
    char *title = g_filename_display_basename(fileName);
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
    {
        // Hack into GtkComboBoxText.
        GtkTreeModel *model = gtk_combo_box_get_model(
            GTK_COMBO_BOX(window->m_currentFile));
        GtkListStore *store = GTK_LIST_STORE(model);
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter_first(model, &iter))
        {
            do
            {
                GValue t = G_VALUE_INIT;
                gtk_tree_model_get_value(model, &iter, 0, &t);
                if (strcmp(g_value_get_string(&t), title) == 0)
                {
                    gtk_list_store_remove(store, &iter);
                    g_value_unset(&t);
                    break;
                }
                g_value_unset(&t);
            }
            while (gtk_tree_model_iter_next(model, &iter));
        }
    }
    g_free(title);
    g_free(fileName);
}

void Window::onCurrentFileChanged(const char *uri)
{
    char *fileName = g_filename_from_uri(uri, NULL, NULL);
    char *title = g_filename_display_basename(fileName);
    // Hack into GtkComboBoxText.
    GtkTreeModel *model = gtk_combo_box_get_model(
        GTK_COMBO_BOX(m_currentFile));
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(model, &iter))
    {
        int i = 0;
        do
        {
            GValue t = G_VALUE_INIT;
            gtk_tree_model_get_value(model, &iter, 0, &t);
            if (strcmp(g_value_get_string(&t), title) == 0)
            {
                gtk_combo_box_set_active(GTK_COMBO_BOX(m_currentFile),
                                         i);
                g_value_unset(&t);
                break;
            }
            g_value_unset(&t);
            ++i;
        }
        while (gtk_tree_model_iter_next(model, &iter));
    }
    g_free(title);
    g_free(fileName);
}

void Window::onCurrentTextEditorCursorChanged(int line, int column)
{
    char *cp;
    cp = g_strdup_printf("%d", line + 1);
    gtk_entry_set_text(GTK_ENTRY(m_currentLine), cp);
    cp = g_strdup_printf("%d", column + 1);
    gtk_entry_set_text(GTK_ENTRY(m_currentColumn), cp);
}

gboolean Window::onWorkerBegunInMainThread(gpointer param)
{
    char *desc = static_cast<char *>(param);
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
    {
        for (int i = 0; i < window->m_workerCount; ++i)
        {
            GValue active = G_VALUE_INIT;
            g_value_init(&active, G_TYPE_BOOLEAN);
            g_object_get_property(G_OBJECT(window->m_workersStatus[i]),
                                  "active", &active);
            if (!g_value_get_boolean(&active))
            {
                gtk_spinner_start(GTK_SPINNER(window->m_workersStatus[i]));
                gtk_widget_set_tooltip_text(window->m_workersStatus[i], desc);
                g_value_unset(&active);
                break;
            }
            g_value_unset(&active);
        }
    }
    return FALSE;
}

gboolean Window::onWorkerEndedInMainThread(gpointer param)
{
    char *desc = static_cast<char *>(param);
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
    {
        for (int i = 0; i < window->m_workerCount; ++i)
        {
            char *d =
                gtk_widget_get_tooltip_text(window->m_workersStatus[i]);
            if (strcmp(d, desc) == 0)
            {
                gtk_spinner_stop(GTK_SPINNER(window->m_workersStatus[i]));
                gtk_widget_set_tooltip_text(window->m_workersStatus[i],
                                            _("Idle"));
                g_free(d);
                break;
            }
            g_free(d);
        }
    }
    g_free(desc);
    return FALSE;
}

void Window::onWorkerBegun(const char *desc)
{
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
                    onWorkerBegunInMainThread,
                    g_strdup(desc),
                    NULL);
}

void Window::onWorkerEnded(const char *desc)
{
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE,
                    onWorkerEndedInMainThread,
                    g_strdup(desc),
                    NULL);
}

}
