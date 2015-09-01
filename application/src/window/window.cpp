// Top-level window.
// Copyright (C) 2011 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "window.hpp"
#include "actions.hpp"
#include "widget/paned.hpp"
#include "widget/widget-with-bars.hpp"
#include "widget/notebook.hpp"
#include "editors/file.hpp"
#include "editors/editor.hpp"
#include "editors/text-editor.hpp"
#include "project/project-explorer.hpp"
#include "build-system/build-log-view-group.hpp"
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
#include <utility>
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
#define LAYOUT "layout"
#define CHILD "child"

#define WINDOW_ID "window"
#define MAIN_AREA_ID "main-area"
#define EDITOR_GROUP_ID "editor-group"
#define PANED_ID "paned"
#define PROJECT_EXPLORER_ID "project-explorer"
#define BUILD_LOG_VIEW_GROUP_ID "build-log-view-group"

#define SIDE_PANE_MENU_ITEM_LABEL "side-pane-menu-item-label"
#define SIDE_PANE_CHILDREN_MENU_LABEL "side-pane-children-menu-label"
#define SIDE_PANE_CHILD_MENU_ITEM_LABEL "side-pane-child-manu-item-label"

namespace
{

const double DEFAULT_SIZE_RATIO = 0.7;

const double SIDE_PANE_SIZE_RATIO = 0.25;

const int LINE_NUMBER_WIDTH = 6;

const int COLUMN_NUMBER_WIDTH = 4;

const int MAX_SIDE_PANE_CHILD_INDEX = 20;

int serialNumber = 0;

Samoyed::Widget *findPane(Samoyed::Widget &widget, const char *id)
{
    if (strcmp(widget.id(), id) == 0)
        return &widget;
    if (strcmp(widget.id(), PANED_ID) != 0)
        return NULL;
    Samoyed::Paned &paned = static_cast<Samoyed::Paned &>(widget);
    Samoyed::Widget *child = findPane(paned.child(0), id);
    if (child)
        return child;
    return findPane(paned.child(1), id);
}

const Samoyed::Widget *findPane(const Samoyed::Widget &widget, const char *id)
{
    if (strcmp(widget.id(), id) == 0)
        return &widget;
    if (strcmp(widget.id(), PANED_ID) != 0)
        return NULL;
    const Samoyed::Paned &paned = static_cast<const Samoyed::Paned &>(widget);
    const Samoyed::Widget *child = findPane(paned.child(0), id);
    if (child)
        return child;
    return findPane(paned.child(1), id);
}

bool closeEmptyEditorGroup(Samoyed::Widget &widget, Samoyed::Widget &root)
{
    if (strcmp(widget.id(), PANED_ID) != 0)
    {
        if (&widget == &root)
            return false;
        if (static_cast<Samoyed::Notebook &>(widget).childCount() == 0)
        {
            widget.close();
            return true;
        }
        return false;
    }
    Samoyed::Paned &paned = static_cast<Samoyed::Paned &>(widget);
    if (closeEmptyEditorGroup(paned.child(0), root))
        return true;
    return closeEmptyEditorGroup(paned.child(1), root);
}

int countEditorGroups(Samoyed::Widget &widget)
{
    if (strcmp(widget.id(), PANED_ID) == 0)
    {
        Samoyed::Paned &paned = static_cast<Samoyed::Paned &>(widget);
        int count = 0;
        for (int i = 0; i < paned.childCount(); ++i)
            count += countEditorGroups(paned.child(i));
        return count;
    }
    return 1;
}

void setEditorGroupCloseOnEmpty(Samoyed::Widget &widget, bool closeOnEmpty)
{
    if (strcmp(widget.id(), PANED_ID) == 0)
    {
        Samoyed::Paned &paned = static_cast<Samoyed::Paned &>(widget);
        int count = 0;
        for (int i = 0; i < paned.childCount(); ++i)
            setEditorGroupCloseOnEmpty(paned.child(i), closeOnEmpty);
    }
    else
        static_cast<Samoyed::Notebook &>(widget).setCloseOnEmpty(closeOnEmpty);
}

void checkEditorGroupsCloseOnEmpty(Samoyed::Window &window)
{
    if (window.mainArea().childCount() > window.mainArea().barCount())
    {
        // If only one editor group is left, disallow it to be closed when
        // empty.
        if (countEditorGroups(window.mainArea().mainChild()) == 1)
            setEditorGroupCloseOnEmpty(window.mainArea().mainChild(), false);
        // If there are multiple editor groups, let them be closed when empty.
        else
            setEditorGroupCloseOnEmpty(window.mainArea().mainChild(), true);
    }
}

void addEditorGroupClosedCallbacks(Samoyed::Window &window,
                                   Samoyed::Widget &widget)
{
    if (strcmp(widget.id(), PANED_ID) == 0)
    {
        Samoyed::Paned &paned = static_cast<Samoyed::Paned &>(widget);
        addEditorGroupClosedCallbacks(window, paned.child(0));
        addEditorGroupClosedCallbacks(window, paned.child(1));
    }
    else
        widget.addClosedCallback(boost::bind(checkEditorGroupsCloseOnEmpty,
                                             boost::ref(window)));
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

void onWorkerBegun(const boost::shared_ptr<Samoyed::Worker> &worker)
{
    Samoyed::Window::addMessage(worker->description());
}

void onWorkerEnded(const boost::shared_ptr<Samoyed::Worker> &worker)
{
    Samoyed::Window::removeMessage(worker->description());
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
                                        std::list<std::string> *errors)
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
                if (errors)
                {
                    cp = g_strdup_printf(
                        _("Line %d: More than one \"%s\" elements seen.\n"),
                        child->line, WIDGET_CONTAINER);
                    errors->push_back(cp);
                    g_free(cp);
                }
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
                    m_configuration.screenIndex =
                        boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid integer \"%s\" for element "
                              "\"%s\". %s.\n"),
                            child->line, value, SCREEN_INDEX, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                    m_configuration.width = boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid integer \"%s\" for element "
                              "\"%s\". %s.\n"),
                            child->line, value, WIDTH, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                    m_configuration.height = boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid integer \"%s\" for element "
                              "\"%s\". %s.\n"),
                            child->line, value, HEIGHT, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                    m_configuration.inFullScreen =
                        boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                            child->line, value, IN_FULL_SCREEN, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                    m_configuration.maximized =
                        boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                            child->line, value, MAXIMIZED, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                    m_configuration.toolbarVisible =
                        boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                            child->line, value, TOOLBAR_VISIBLE, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                    m_configuration.statusBarVisible =
                        boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                            child->line, value, STATUS_BAR_VISIBLE,
                            error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                    m_configuration.toolbarVisibleInFullScreen =
                        boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                            child->line, value, TOOLBAR_VISIBLE_IN_FULL_SCREEN,
                            error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        LAYOUT) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_configuration.layout =
                        static_cast<Layout>(
                            boost::lexical_cast<unsigned int>(value));
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid nonnegative integer \"%s\" for "
                              "element \"%s\". %s.\n"),
                            child->line, value, LAYOUT,
                            error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                        if (errors)
                        {
                            cp = g_strdup_printf(
                                _("Line %d: More than one children contained "
                                  "by the bin.\n"),
                                grandChild->line);
                            errors->push_back(cp);
                            g_free(cp);
                        }
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
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: \"%s\" element missing.\n"),
                node->line, WIDGET_CONTAINER);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }
    if (!m_child)
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: No child contained by the bin.\n"),
                node->line);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }
    return true;
}

Window::XmlElement *Window::XmlElement::read(xmlNodePtr node,
                                             std::list<std::string> *errors)
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
    str = boost::lexical_cast<std::string>(m_configuration.screenIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(SCREEN_INDEX),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.width);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(WIDTH),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.height);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(HEIGHT),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.inFullScreen);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(IN_FULL_SCREEN),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.maximized);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(MAXIMIZED),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.toolbarVisible);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(TOOLBAR_VISIBLE),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.statusBarVisible);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(STATUS_BAR_VISIBLE),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(
        m_configuration.toolbarVisibleInFullScreen);
    xmlNewTextChild(
        node, NULL,
        reinterpret_cast<const xmlChar *>(TOOLBAR_VISIBLE_IN_FULL_SCREEN),
        reinterpret_cast<const xmlChar *>(str.c_str()));
    str = boost::lexical_cast<std::string>(m_configuration.layout);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(LAYOUT),
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
    m_bypassCurrentFileChange(false),
    m_bypassCurrentFileInput(false),
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

    // By default show all.
    gtk_widget_show_all(grid);

    // Configure the window.
    if (config.screenIndex >= 0)
    {
        GdkDisplay *display = gdk_display_get_default();
        GdkScreen *screen = gdk_display_get_screen(display,
                                                   config.screenIndex);
        gtk_window_set_screen(GTK_WINDOW(window), screen);
    }

    m_width = config.width;
    m_height = config.height;
    if (m_width == -1)
    {
        GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(window));
        m_width = gdk_screen_get_width(screen) * DEFAULT_SIZE_RATIO;
        m_height = gdk_screen_get_height(screen) * DEFAULT_SIZE_RATIO;
    }
    gtk_window_set_default_size(GTK_WINDOW(window), m_width, m_height);

    m_toolbarVisible = config.toolbarVisible;
    m_statusBarVisible = config.statusBarVisible;
    m_toolbarVisibleInFullScreen = config.toolbarVisibleInFullScreen;
    if (!m_toolbarVisible)
        setToolbarVisible(false);
    if (!m_statusBarVisible)
        setStatusBarVisible(false);
    if (config.inFullScreen)
        enterFullScreen();
    else if (config.maximized)
        gtk_window_maximize(GTK_WINDOW(window));

    return true;
}

bool Window::setup(const char *sessionName, const Configuration &config)
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

    // Create the navigation pane and the tools pane.
    createNavigationPane(config.layout);
    createToolsPane(config.layout);

    // Set the title.
    std::string title(sessionName);
    title += _(" - Samoyed IDE");
    setTitle(title.c_str());
    gtk_window_set_title(GTK_WINDOW(gtkWidget()), title.c_str());

    m_actions->updateStatefulActions();

    s_created(*this);
    return true;
}

Window *Window::create(const char *sessionName, const Configuration &config)
{
    Window *window = new Window;
    if (!window->setup(sessionName, config))
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

    // Validate the restored window.  Close editor groups that don't contain
    // any editor.
    for (Widget *w = &mainArea().mainChild();
         closeEmptyEditorGroup(*w, *w);
         w = &mainArea().mainChild())
        ;

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

    checkEditorGroupsCloseOnEmpty(*this);
    addEditorGroupClosedCallbacks(*this, mainArea().mainChild());

    createMenuItemsForSidePanesRecursively(*child);

    gtk_window_set_title(GTK_WINDOW(gtkWidget()), title());

    m_actions->updateStatefulActions();

    s_restored(*this);

    grabFocus();
    gtk_widget_show(gtkWidget());
    return true;
}

Window::~Window()
{
    assert(!m_child);
    for (std::vector<FileTitleUri>::iterator it = m_fileTitlesUris.begin();
         it != m_fileTitlesUris.end();
         ++it)
        g_free(it->title);
    Application::instance().removeWindow(*this);
    if (m_uiManager)
        g_object_unref(m_uiManager);
}

void Window::destroy()
{
    onClosed();
    Application::instance().destroyWindow(*this);
}

gboolean Window::onDeleteEvent(GtkWidget *widget,
                               GdkEvent *event,
                               Window *window)
{
    if (!Application::instance().windows()->next())
    {
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
    Application::instance().setCurrentWindow(window);
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

    checkEditorGroupsCloseOnEmpty(*this);
    newEditorGroup->addClosedCallback(boost::bind(checkEditorGroupsCloseOnEmpty,
                                                  boost::ref(*this)));
    return newEditorGroup;
}

void Window::addEditorToEditorGroup(Editor &editor, Notebook &editorGroup,
                                    int index)
{
    editorGroup.addChild(editor, index);
}

void Window::createNavigationPane(Layout layout)
{
    Notebook *pane =
        Notebook::create(NAVIGATION_PANE_ID, NULL, false, false, true);
    pane->setTitle(_("_Navigation Pane"));
    pane->setProperty(SIDE_PANE_MENU_ITEM_LABEL, _("_Navigation Pane"));
    pane->setProperty(SIDE_PANE_CHILDREN_MENU_LABEL, _("Na_vigators"));
    addSidePane(*pane, mainArea(), SIDE_LEFT,
                m_width * SIDE_PANE_SIZE_RATIO);
    s_navigationPaneCreated(*pane);
}

void Window::createToolsPane(Layout layout)
{
    Notebook *pane =
        Notebook::create(TOOLS_PANE_ID, NULL, false, false, true);
    pane->setTitle(_("_Tools Pane"));
    pane->setProperty(SIDE_PANE_MENU_ITEM_LABEL, _("_Tools Pane"));
    pane->setProperty(SIDE_PANE_CHILDREN_MENU_LABEL, _("T_ools"));
    addSidePane(*pane, mainArea(),
                layout == LAYOUT_TOOLS_PANE_RIGHT ? SIDE_RIGHT : SIDE_BOTTOM,
                (layout == LAYOUT_TOOLS_PANE_RIGHT ? m_width : m_height)
                * SIDE_PANE_SIZE_RATIO);
    s_toolsPaneCreated(*pane);
}

void Window::createMenuItemForSidePane(Widget &pane)
{
    SidePaneData *data = new SidePaneData;

    std::string actionName("show-hide-");
    actionName += pane.id();

    const std::string *label = pane.getProperty(SIDE_PANE_MENU_ITEM_LABEL);
    std::string str;
    if (!label)
    {
        // To be safe, remove all underscores.
        str = pane.title();
        str.erase(std::remove(str.begin(), str.end(), '_'), str.end());
        label = &str;
    }

    std::string tooltip(pane.title());
    tooltip.erase(std::remove(tooltip.begin(), tooltip.end(), '_'),
                  tooltip.end());
    std::transform(tooltip.begin(), tooltip.end(), tooltip.begin(),
                   tolower);
    tooltip.insert(0, _("Show or hide "));

    GtkToggleAction *action = gtk_toggle_action_new(actionName.c_str(),
                                                    label->c_str(),
                                                    tooltip.c_str(),
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

    const std::string *label2 =
        pane.getProperty(SIDE_PANE_CHILDREN_MENU_LABEL);
    if (!label2)
    {
        // To be safe, remove all underscores.
        str = pane.title();
        str.erase(std::remove(str.begin(), str.end(), '_'), str.end());
        str += _(" Children");
        label2 = &str;
    }

    GtkAction *action2 = gtk_action_new(actionName2.c_str(),
                                        label2->c_str(),
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
    for (int i = 1; i <= MAX_SIDE_PANE_CHILD_INDEX; ++i)
    {
        std::string placeHolder("placeholder-");
        placeHolder += boost::lexical_cast<std::string>(i);
        gtk_ui_manager_add_ui(m_uiManager,
                              mergeId,
                              uiPath.c_str(),
                              placeHolder.c_str(),
                              NULL,
                              GTK_UI_MANAGER_PLACEHOLDER,
                              FALSE);
    }

    pane.addClosedCallback(boost::bind(onSidePaneClosed, this, _1));

    data->action = action;
    data->action2 = action2;
    data->signalHandlerId = signalHandlerId;
    data->uiMergeId = mergeId;

    m_sidePaneData.insert(std::make_pair(pane.id(), data));
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
                                   const char *label)
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

    std::string tooltip(label);
    tooltip.erase(std::remove(tooltip.begin(), tooltip.end(), '_'),
                  tooltip.end());
    std::transform(tooltip.begin(), tooltip.end(), tooltip.begin(),
                   tolower);
    tooltip.insert(0, _("Open or close "));

    GtkToggleAction *action = gtk_toggle_action_new(actionName.c_str(),
                                                    label,
                                                    tooltip.c_str(),
                                                    NULL);
    gtk_toggle_action_set_active(action, pane->findChild(id) ? TRUE : FALSE);
    g_signal_connect(action, "toggled",
                     G_CALLBACK(openCloseSidePaneChild), this);
    gtk_action_group_add_action(m_actions->actionGroup(), GTK_ACTION(action));
    g_object_unref(action);

    std::string uiPath("/main-menu-bar/view/side-panes/");
    uiPath += paneId;
    uiPath += "-children/placeholder-";
    uiPath += boost::lexical_cast<std::string>(index);
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
    // If the child is open, close it.
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
        child->addClosedCallback(
            boost::bind(onSidePaneChildClosed, this, _1, boost::cref(*pane)));
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
    m_inFullScreen = true;
    bool toolbarVisible = gtk_widget_get_visible(m_toolbar);
    if (toolbarVisible)
    {
        if (!m_toolbarVisibleInFullScreen)
            gtk_widget_set_visible(m_toolbar, false);
    }
    else
    {
        if (m_toolbarVisibleInFullScreen)
            gtk_widget_set_visible(m_toolbar, true);
    }
    gtk_widget_set_visible(m_menuBar, false);
    gtk_widget_set_visible(m_statusBar, false);
    gtk_window_fullscreen(GTK_WINDOW(gtkWidget()));
    m_actions->onWindowFullScreenChanged(true);
}

void Window::leaveFullScreen()
{
    m_inFullScreen = false;
    bool toolbarVisibleInFullScreen = gtk_widget_get_visible(m_toolbar);
    if (toolbarVisibleInFullScreen)
    {
        if (!m_toolbarVisible)
            gtk_widget_set_visible(m_toolbar, false);
    }
    else
    {
        if (m_toolbarVisible)
            gtk_widget_set_visible(m_toolbar, true);
    }
    gtk_widget_set_visible(m_menuBar, true);
    gtk_widget_set_visible(m_statusBar, true);
    gtk_window_unfullscreen(GTK_WINDOW(gtkWidget()));
    m_actions->onWindowFullScreenChanged(false);
}

Window::Layout Window::layout() const
{
    if (static_cast<const Paned *>(toolsPane().parent())->orientation() ==
        Paned::ORIENTATION_HORIZONTAL)
        return LAYOUT_TOOLS_PANE_RIGHT;
    return LAYOUT_TOOLS_PANE_BOTTOM;
}

void Window::changeLayout(Layout layout)
{
    if (layout == LAYOUT_TOOLS_PANE_RIGHT)
        static_cast<Paned *>(mainArea().parent())->setOrientation(
            Paned::ORIENTATION_HORIZONTAL);
    else
        static_cast<Paned *>(mainArea().parent())->setOrientation(
            Paned::ORIENTATION_VERTICAL);
}

Window::Configuration Window::configuration() const
{
    Configuration config;
    config.screenIndex =
        gdk_screen_get_number(gtk_window_get_screen(GTK_WINDOW(gtkWidget())));
    config.width = m_width;
    config.height = m_height;
    config.inFullScreen = m_inFullScreen;
    config.maximized = m_maximized;
    config.toolbarVisible = m_toolbarVisible;
    config.statusBarVisible = m_statusBarVisible;
    config.toolbarVisibleInFullScreen = m_toolbarVisibleInFullScreen;
    config.layout = layout();
    return config;
}

void Window::onToolbarVisibilityChanged(GtkWidget *toolbar,
                                        GParamSpec *spec,
                                        Window *window)
{
    bool visible = gtk_widget_get_visible(toolbar);
    if (window->inFullScreen())
        window->m_toolbarVisibleInFullScreen = visible;
    else
        window->m_toolbarVisible = visible;
    window->m_actions->onToolbarVisibilityChanged(visible);
}

void Window::onStatusBarVisibilityChanged(GtkWidget *statusBar,
                                          GParamSpec *spec,
                                          Window *window)
{
    bool visible = gtk_widget_get_visible(statusBar);
    if (!window->inFullScreen())
        window->m_statusBarVisible = visible;
    window->m_actions->onStatusBarVisibilityChanged(visible);
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

void Window::addUiForAction(const char *name,
                            const char *path,
                            guint &mergeId,
                            guint &mergeIdSeparator,
                            bool separate)
{
    if (separate)
    {
        mergeIdSeparator = gtk_ui_manager_new_merge_id(m_uiManager);
        std::string separatorName(name);
        separatorName += "-separator";
        gtk_ui_manager_add_ui(m_uiManager,
                              mergeIdSeparator,
                              path,
                              separatorName.c_str(),
                              NULL,
                              GTK_UI_MANAGER_SEPARATOR,
                              FALSE);
    }

    mergeId = gtk_ui_manager_new_merge_id(m_uiManager);
    gtk_ui_manager_add_ui(m_uiManager,
                          mergeId,
                          path,
                          name,
                          name,
                          GTK_UI_MANAGER_AUTO,
                          FALSE);
}

GtkAction *Window::addAction(
    const char *name,
    const char *path,
    const char *path2,
    const char *label,
    const char *tooltip,
    const char *iconName,
    const char *accelerator,
    const boost::function<void (Window &, GtkAction *)> &activate,
    const boost::function<bool (Window &, GtkAction *)> &sensitive,
    bool separate)
{
    ActionData *data = new ActionData;
    data->activate = boost::bind(activate, boost::ref(*this), _1);
    data->sensitive = boost::bind(sensitive, boost::ref(*this), _1);

    GtkAction *action =
        gtk_action_new(name, label, tooltip, NULL);
    gtk_action_set_icon_name(action, iconName);
    g_signal_connect(action, "activate",
                     G_CALLBACK(::activateAction), &data->activate);
    gtk_action_group_add_action_with_accel(m_actions->actionGroup(),
                                           action,
                                           accelerator);
    g_object_unref(action);
    data->action = action;

    addUiForAction(name, path,
                   data->uiMergeId, data->uiMergeIdSeparator,
                   separate);
    if (path2)
        addUiForAction(name, path2,
                       data->uiMergeId2, data->uiMergeIdSeparator2,
                       separate);

    m_actionData[name] = data;
    return action;
}

GtkToggleAction *Window::addToggleAction(
    const char *name,
    const char *path,
    const char *path2,
    const char *label,
    const char *tooltip,
    const char *iconName,
    const char *accelerator,
    const boost::function<void (Window &, GtkToggleAction *)> &toggled,
    const boost::function<bool (Window &, GtkAction *)> &sensitive,
    bool activeByDefault,
    bool separate)
{
    ActionData *data = new ActionData;
    data->toggled = boost::bind(toggled, boost::ref(*this), _1);
    data->sensitive = boost::bind(sensitive, boost::ref(*this), _1);

    GtkToggleAction *action =
        gtk_toggle_action_new(name, label, tooltip, NULL);
    gtk_action_set_icon_name(GTK_ACTION(action), iconName);
    gtk_toggle_action_set_active(action, activeByDefault);
    g_signal_connect(action, "toggled",
                     G_CALLBACK(onActionToggled), &data->toggled);
    gtk_action_group_add_action_with_accel(m_actions->actionGroup(),
                                           GTK_ACTION(action),
                                           accelerator);
    g_object_unref(action);
    data->action = GTK_ACTION(action);

    addUiForAction(name, path,
                   data->uiMergeId, data->uiMergeIdSeparator,
                   separate);
    if (path2)
        addUiForAction(name, path2,
                       data->uiMergeId2, data->uiMergeIdSeparator2,
                       separate);

    m_actionData[name] = data;
    return action;
}

void Window::removeAction(const char *name)
{
    ActionData *data = m_actionData[name];
    gtk_ui_manager_remove_ui(m_uiManager, data->uiMergeId);
    if (data->uiMergeIdSeparator != static_cast<guint>(-1))
        gtk_ui_manager_remove_ui(m_uiManager, data->uiMergeIdSeparator);
    if (data->uiMergeId2 != static_cast<guint>(-1))
        gtk_ui_manager_remove_ui(m_uiManager, data->uiMergeId2);
    if (data->uiMergeIdSeparator2 != static_cast<guint>(-1))
        gtk_ui_manager_remove_ui(m_uiManager, data->uiMergeIdSeparator2);
    gtk_action_group_remove_action(m_actions->actionGroup(), data->action);
    m_actionData.erase(name);
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

    // Add already open files.
    for (File *file = Application::instance().files();
         file;
         file = file->next())
    {
        char *fileName = g_filename_from_uri(file->uri(), NULL, NULL);
        FileTitleUri titleUri;
        titleUri.title = g_filename_display_basename(fileName);
        titleUri.uri = file->uri();
        std::vector<FileTitleUri>::iterator it =
            std::lower_bound(m_fileTitlesUris.begin(), m_fileTitlesUris.end(),
                             titleUri, compareFileTitles);
        gtk_combo_box_text_insert_text(GTK_COMBO_BOX_TEXT(m_currentFile),
                                       it - m_fileTitlesUris.begin(),
                                       titleUri.title);
        m_fileTitlesUris.insert(it, titleUri);
        g_free(fileName);
    }

    gtk_widget_set_tooltip_text(
        m_currentFile,
        _("Select the file to edit"));
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            m_currentFile, label,
                            GTK_POS_RIGHT, 1, 1);

    g_signal_connect(m_currentFile, "changed",
                     G_CALLBACK(onCurrentFileInput), this);

    label = gtk_label_new(_("Line:"));
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            label, m_currentFile,
                            GTK_POS_RIGHT, 1, 1);
    m_currentLine = gtk_entry_new();
    gtk_entry_set_width_chars(GTK_ENTRY(m_currentLine),
                              LINE_NUMBER_WIDTH);
#if GTK_CHECK_VERSION(3, 12, 0)
    gtk_entry_set_max_width_chars(GTK_ENTRY(m_currentLine),
                                  LINE_NUMBER_WIDTH);
#endif
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
#if GTK_CHECK_VERSION(3, 12, 0)
    gtk_entry_set_max_width_chars(GTK_ENTRY(m_currentColumn),
                                  COLUMN_NUMBER_WIDTH);
#endif
    gtk_widget_set_tooltip_text(
        m_currentColumn,
        _("Input the column number to go"));
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            m_currentColumn, label,
                            GTK_POS_RIGHT, 1, 1);

    g_signal_connect(m_currentLine, "activate",
                     G_CALLBACK(onCurrentTextEditorCursorInput), this);
    g_signal_connect(m_currentColumn, "activate",
                     G_CALLBACK(onCurrentTextEditorCursorInput), this);

    m_message = gtk_label_new(NULL);
    gtk_label_set_ellipsize(GTK_LABEL(m_message), PANGO_ELLIPSIZE_END);
    gtk_grid_attach_next_to(GTK_GRID(m_statusBar),
                            m_message, m_currentColumn,
                            GTK_POS_RIGHT, 1, 1);

    gtk_grid_set_column_spacing(GTK_GRID(m_statusBar), CONTAINER_SPACING);
    gtk_widget_set_margin_left(m_statusBar, CONTAINER_SPACING);
    gtk_widget_set_margin_right(m_statusBar, CONTAINER_SPACING);

    gtk_widget_set_hexpand(m_statusBar, TRUE);
    g_signal_connect_after(m_statusBar, "notify::visible",
                           G_CALLBACK(onStatusBarVisibilityChanged), this);
}

gboolean Window::addMessageInMainThread(gpointer param)
{
    char *message = static_cast<char *>(param);
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
    {
        if (!window->m_message)
            continue;
        const char *oldMsg = gtk_label_get_text(GTK_LABEL(window->m_message));
        if (*oldMsg)
        {
            std::string msg = oldMsg;
            msg += ' ';
            msg += message;
            gtk_label_set_text(GTK_LABEL(window->m_message), msg.c_str());
            gtk_widget_set_tooltip_text(window->m_message, msg.c_str());
        }
        else
        {
            gtk_label_set_text(GTK_LABEL(window->m_message), message);
            gtk_widget_set_tooltip_text(window->m_message, message);
        }
    }
    g_free(message);
    return FALSE;
}

gboolean Window::removeMessageInMainThread(gpointer param)
{
    char *message = static_cast<char *>(param);
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
    {
        if (!window->m_message)
            continue;
        const char *oldMsg = gtk_label_get_text(GTK_LABEL(window->m_message));
        const char *found = strstr(oldMsg, message);
        if (!found)
            continue;
        int len = strlen(message);
        if (found == oldMsg)
        {
            // Exclude this message.
            found += len;
            // Exclude the delimiter, if any.
            if (*found == ' ')
                found++;
            // Note that 'found' can't be used after the label is changed.
            gtk_widget_set_tooltip_text(window->m_message, found);
            gtk_label_set_text(GTK_LABEL(window->m_message), found);
        }
        else if (*(found - 1) == ' ')
        {
            // Exclude the delimiter.
            std::string msg(oldMsg, found - oldMsg - 1);
            msg += found + len;
            gtk_label_set_text(GTK_LABEL(window->m_message), msg.c_str());
            gtk_widget_set_tooltip_text(window->m_message, msg.c_str());
        }
        else
        {
            std::string msg(oldMsg, found - oldMsg);
            msg += found + len;
            gtk_label_set_text(GTK_LABEL(window->m_message), msg.c_str());
            gtk_widget_set_tooltip_text(window->m_message, msg.c_str());
        }
    }
    g_free(message);
    return FALSE;
}

void Window::addMessage(const char *message)
{
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, addMessageInMainThread,
                    g_strdup(message), NULL);
}

void Window::removeMessage(const char *message)
{
    g_idle_add_full(G_PRIORITY_DEFAULT_IDLE, removeMessageInMainThread,
                    g_strdup(message), NULL);
}

void Window::onFileOpened(const char *uri)
{
    char *fileName = g_filename_from_uri(uri, NULL, NULL);
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
    {
        // The window has not been setup completely in the early stage of
        // restoring a session.
        if (window->m_currentFile)
        {
            FileTitleUri titleUri;
            titleUri.title = g_filename_display_basename(fileName);
            titleUri.uri = uri;
            std::vector<FileTitleUri>::iterator it =
                std::lower_bound(window->m_fileTitlesUris.begin(),
                                 window->m_fileTitlesUris.end(),
                                 titleUri, compareFileTitles);
            gtk_combo_box_text_insert_text(
                GTK_COMBO_BOX_TEXT(window->m_currentFile),
                it - window->m_fileTitlesUris.begin(),
                titleUri.title);
            window->m_fileTitlesUris.insert(it, titleUri);
        }
    }
    g_free(fileName);
}

void Window::onFileClosed(const char *uri)
{
    char *fileName = g_filename_from_uri(uri, NULL, NULL);
    FileTitleUri titleUri;
    titleUri.title = g_filename_display_basename(fileName);
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
    {
        std::pair<std::vector<FileTitleUri>::iterator,
                  std::vector<FileTitleUri>::iterator> p =
            std::equal_range(window->m_fileTitlesUris.begin(),
                             window->m_fileTitlesUris.end(),
                             titleUri, compareFileTitles);
        gtk_combo_box_text_remove(
            GTK_COMBO_BOX_TEXT(window->m_currentFile),
            p.first - window->m_fileTitlesUris.begin());
        g_free(p.first->title);
        window->m_fileTitlesUris.erase(p.first);
    }
    g_free(titleUri.title);
    g_free(fileName);
}

void Window::onCurrentFileChanged(const char *uri)
{
    if (m_bypassCurrentFileChange)
        return;

    char *fileName = g_filename_from_uri(uri, NULL, NULL);
    FileTitleUri titleUri;
    titleUri.title = g_filename_display_basename(fileName);
    std::pair<std::vector<FileTitleUri>::iterator,
              std::vector<FileTitleUri>::iterator> p =
        std::equal_range(m_fileTitlesUris.begin(),
                         m_fileTitlesUris.end(),
                         titleUri, compareFileTitles);
    m_bypassCurrentFileInput = true;
    gtk_combo_box_set_active(GTK_COMBO_BOX(m_currentFile),
                             p.first - m_fileTitlesUris.begin());
    m_bypassCurrentFileInput = false;
    g_free(titleUri.title);
    g_free(fileName);
}

void Window::onCurrentTextEditorCursorChanged(int line, int column)
{
    char *cp;
    cp = g_strdup_printf("%d", line + 1);
    gtk_entry_set_text(GTK_ENTRY(m_currentLine), cp);
    g_free(cp);
    cp = g_strdup_printf("%d", column + 1);
    gtk_entry_set_text(GTK_ENTRY(m_currentColumn), cp);
    g_free(cp);
}

void Window::onCurrentFileInput(GtkComboBox *combo, Window *window)
{
    if (window->m_bypassCurrentFileInput)
        return;

    int index = gtk_combo_box_get_active(combo);
    if (index < 0)
        return;

    std::vector<FileTitleUri>::iterator it =
        window->m_fileTitlesUris.begin() + index;

    // Check to see if the file is open in this window.  If any, switch to the
    // editor.
    File *file = Application::instance().findFile(it->uri);
    for (Editor *editor = file->editors();
         editor;
         editor = editor->nextInFile())
    {
        Widget *win = editor;
        for (Widget *p = win; p; p = win->parent())
            win = p;
        if (win == window)
        {
            window->m_bypassCurrentFileChange = true;
            editor->setCurrent();
            window->m_bypassCurrentFileChange = false;
            return;
        }
    }

    // Open the file in this window.
    Editor *editor = file->createEditor(NULL);
    window->m_bypassCurrentFileChange = true;
    window->addEditorToEditorGroup(
        *editor,
        window->currentEditorGroup(),
        window->currentEditorGroup().currentChildIndex() + 1);
    editor->setCurrent();
    window->m_bypassCurrentFileChange = false;
}

void Window::onCurrentTextEditorCursorInput(GtkEntry *entry, Window *window)
{
    Notebook &editorGroup = window->currentEditorGroup();
    if (editorGroup.childCount() > 0)
    {
        Samoyed::TextEditor &editor =
            static_cast<TextEditor &>(editorGroup.currentChild());
        const char *line = gtk_entry_get_text(GTK_ENTRY(window->m_currentLine));
        const char *column =
            gtk_entry_get_text(GTK_ENTRY(window->m_currentColumn));
        int ln, col;
        try
        {
            ln = boost::lexical_cast<int>(line);
        }
        catch (boost::bad_lexical_cast &)
        {
            ln = -1;
        }
        try
        {
            col = boost::lexical_cast<int>(column);
        }
        catch (boost::bad_lexical_cast &)
        {
            col = -1;
        }
        if (ln >= 1 && col >= 1)
        {
            ln--;
            col--;
            if (ln >= editor.lineCount())
                ln = editor.lineCount() - 1;
            if (col > editor.maxColumnInLine(ln))
                col = 0;
            editor.setCursor(ln, col);
        }
        editor.setCurrent();
    }
}

bool Window::compareFileTitles(const FileTitleUri &titleUri1,
                               const FileTitleUri &titleUri2)
{
    return strcmp(titleUri1.title, titleUri2.title) < 0;
}

boost::signals2::connection Window::s_workerBegunConn,
                            Window::s_workerEndedConn;

void Window::enableShowActiveWorkers()
{
    s_workerBegunConn = Worker::addBegunCallbackForAny(onWorkerBegun);
    s_workerEndedConn = Worker::addBegunCallbackForAny(onWorkerEnded);
}

void Window::disableShowActiveWorkers()
{
    s_workerBegunConn.disconnect();
    s_workerEndedConn.disconnect();
}

ProjectExplorer *Window::projectExplorer()
{
    Notebook &navig = navigationPane();
    for (int i = 0; i < navig.childCount(); ++i)
    {
        if (strcmp(navig.child(i).id(), PROJECT_EXPLORER_ID) == 0)
            return static_cast<ProjectExplorer *>(&navig.child(i));
    }
    return NULL;
}

const ProjectExplorer *Window::projectExplorer() const
{
    const Notebook &navig = navigationPane();
    for (int i = 0; i < navig.childCount(); ++i)
    {
        if (strcmp(navig.child(i).id(), PROJECT_EXPLORER_ID) == 0)
            return static_cast<const ProjectExplorer *>(&navig.child(i));
    }
    return NULL;
}

Project *Window::currentProject()
{
    ProjectExplorer *explorer = projectExplorer();
    if (explorer)
        return explorer->currentProject();
    return Application::instance().projects();
}

const Project *Window::currentProject() const
{
    const ProjectExplorer *explorer = projectExplorer();
    if (explorer)
        return explorer->currentProject();
    return Application::instance().projects();
}

BuildLogViewGroup *Window::buildLogViewGroup()
{
    Notebook &tools = toolsPane();
    for (int i = 0; i < tools.childCount(); ++i)
    {
        if (strcmp(tools.child(i).id(), BUILD_LOG_VIEW_GROUP_ID) == 0)
            return static_cast<BuildLogViewGroup *>(&tools.child(i));
    }
    return NULL;
}

const BuildLogViewGroup *Window::buildLogViewGroup() const
{
    const Notebook &tools = toolsPane();
    for (int i = 0; i < tools.childCount(); ++i)
    {
        if (strcmp(tools.child(i).id(), BUILD_LOG_VIEW_GROUP_ID) == 0)
            return static_cast<const BuildLogViewGroup *>(&tools.child(i));
    }
    return NULL;
}

BuildLogViewGroup *Window::openBuildLogViewGroup()
{
    BuildLogViewGroup *group = buildLogViewGroup();
    if (group)
        return group;
    return static_cast<BuildLogViewGroup *>(
        openSidePaneChild(TOOLS_PANE_ID, BUILD_LOG_VIEW_GROUP_ID));
}

}
