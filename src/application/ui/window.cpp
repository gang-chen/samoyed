// Top-level window.
// Copyright (C) 2011 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "window.hpp"
#include "actions.hpp"
#include "pane-base.hpp"
#include "project-explorer.hpp"
#include "editor-group.hpp"
#include "bar.hpp"
#include "../application.hpp"
#include "../utilities/misc.hpp"
#include <assert.h>
#include <gtk/gtk.h>

namespace Samoyed
{

Window::Window(const Configuration &config, PaneBase &content):
    m_content(&content),
    m_firstBar(NULL),
    m_lastBar(NULL),
    m_window(NULL),
    m_mainVBox(NULL),
    m_mainHBox(NULL),
    m_menuBar(NULL),
    m_toolbar(NULL),
    m_uiManager(NULL),
    m_actions(NULL)
{
    m_uiManager = gtk_ui_manager_new();
    gtk_ui_manager_insert_action_group(m_uiManager, m_actions.actionGroup(), 0);

    std::string uiFile(Application::instance().dataDirectory());
    uiFile += G_DIR_SEPARATOR_S "actions-ui.xml";
    // Ignore the possible failure, which implies the installation is broken.
    gtk_ui_manager_add_ui_from_file(m_uiManager, uiFile.c_str(), NULL);

    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    m_mainVBox = gtk_grid_new();
    m_mainHBox = gtk_grid_new();

    gtk_container_add(GTK_CONTAINER(m_window), m_mainVBox);

    m_menuBar = gtk_ui_manager_get_widget(m_uiManager, "/main-menu-bar");
    gtk_grid_attach_next_to(GTK_GRID(m_mainVBox),
                            m_menuBar,
                            NULL,
                            GTK_POS_BOTTOM,
                            1,
                            1);

    m_toolbar = gtk_ui_manager_get_widget(m_uiManager, "/main-toolbar");

    GtkWidget *newPopupMenu = gtk_ui_manager_get_widget(m_uiManager,
                                                        "/new-popup-menu");
    GtkWidget *newIcon = gtk_image_new_from_file();
    GtkWidget *newItem = gtk_menu_tool_button_new(newIcon, NULL);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(newItem),
                                   _("Create an object"));
    gtk_tool_item_set_menu(GTK_TOOL_ITEM(newItem), newPopupMenu);
    gtk_toolbar_insert(GTK_TOOLBAR(m_toolbar), GTK_TOOL_ITEM(newItem), 0);

    gtk_grid_attach_next_to(GTK_GRID(m_mainVBox),
                            m_toolbar,
                            m_menuBar,
                            GTK_POS_BOTTOM,
                            1,
                            1);

    gtk_grid_attach_next_to(GTK_GRID(m_mainVBox),
                            m_mainHBox,
                            m_toolbar,
                            GTK_POS_BOTTOM,
                            1,
                            1);

    gtk_grid_attach_next_to(GTK_GRID(m_mainHBox),
                            m_content.gtkWidget(),
                            NULL,
                            GTK_POS_RIGHT,
                            1,
                            1);

    g_signal_connect(m_window,
                     "delete-event",
                     G_CALLBACK(onDeleteEvent),
                     this);
    g_signal_connect(m_window,
                     "focus-in-event",
                     G_CALLBACK(onFocusInEvent),
                     this);

    gtk_widget_show_all(m_window);

    Application::instance().addWindow(*this);
}

Window::~Window()
{
    assert(!m_content);
    assert(!m_firstBar);
    assert(!m_lastBar);
    Application::instance().removeWindow(*this);
    if (m_uiManager)
        g_object_unref(m_uiManager);
    gtk_widget_destroy(m_window);
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
            Application::instance().currentWindow().gtkWidget() : NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("You will quit Samoyed if you close this window. Continue closing
               this window and quitting Samoyed?"));
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
    m_closing = true;
    for (Bar *bar = m_firstBar, *next; bar; bar = next)
    {
        next = bar->next();
        delete bar;
    }
    if (!m_content->close())
    {
        m_closing = false;
        return TRUE;
    }
    return TRUE;
}

void Window::onContentClosed()
{
    assert(m_closing);
    m_content = NULL;
    delete this;
}

void Window::setContent(PaneBase *content)
{
    if (content)
    {
        assert(!m_content);
        assert(!content->window());
        assert(!content->parent());
        m_content = content;
        m_content->setWindow(this);
        gtk_grid_attach_next_to(GTK_GRID(m_mainHBox),
                                m_content->gtkWidget(),
                                NULL,
                                GTK_POS_RIGHT,
                                1,
                                1);
    }
    else
    {
        assert(m_content);
        assert(m_content->window() == this);
        assert(!m_content->parent());
        gtk_container_remove(GTK_CONTAINER(m_mainHBox), m_content->gtkWidget());
        m_content->setWindow(NULL);
        m_content = NULL;
    }
}

gboolean Window::onFocusInEvent(GtkWidget *widget,
                                GdkEvent *event,
                                gpointer window)
{
    Application::instance().setCurrentWindow(*static_cast<Window *>(window));
    return FALSE;
}

void Window::addBar(Bar &bar)
{
    bar.addToList(m_firstBar, m_lastBar);
    if (bar.orientation() == Bar::ORIENTATION_HORIZONTAL)
        gtk_grid_attach_next_to(GTK_GRID(m_mainVBox),
                                bar.gtkWidget(),
                                NULL,
                                GTK_POS_TOP,
                                1,
                                1);
    else
        gtk_grid_attach_next_to(GTK_GRID(m_mainVBox),
                                bar.gtkWidget(),
                                NULL,
                                GTK_POS_LEFT,
                                1,
                                1);
}

void Window::removeBar(Bar &bar)
{
    bar.removeFromList(m_firstBar, m_lastBar);
}

}
