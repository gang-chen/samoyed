// Top-level window.
// Copyright (C) 2011 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "window.hpp"
#include "actions.hpp"
#include "pane-base.hpp"
#include "project-explorer.hpp"
#include "editor-groups.hpp"
#include "../application.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

Window::Window(const Configuration &config, PaneBase &content):
    m_content(&content),
    m_window(NULL),
    m_mainBox(NULL),
    m_menuBar(NULL),
    m_toolbar(NULL),
    m_uiManager(NULL),
    m_basicActions(NULL),
    m_actionsForProjects(NULL),
    m_actionsForFiles(NULL)
{
    m_uiManager = gtk_ui_manager_new();
    m_basicActions = gtk_action_group_new("basic actions");
    gtk_action_group_set_translation_domain(m_basicActions, NULL);
    gtk_action_group_add_actions(m_basicActions,
                                 Actions::s_basicActionEntries,
                                 G_N_ELEMENTS(Actions::s_basicActionEntries),
                                 window);
    gtk_ui_manager_insert_action_group(m_uiManager, m_basicActions, 0);
    g_object_unref(m_basicActions);

    std::string uiFile(Application::instance().dataDirectory());
    uiFile += G_DIR_SEPARATOR_S "actions-ui.xml";
    // Ignore the possible failure, which implies the installation is broken.
    gtk_ui_manager_add_ui_from_file(m_uiManager, uiFile.c_str(), NULL);

    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    m_mainBox = gtk_vbox_new();
    gtk_container_add(GTK_CONTAINER(m_window), m_mainBox);

    m_menuBar = gtk_ui_manager_get_widget(m_uiManager, "/main-menu-bar");
    gtk_box_pack_start(GTK_BOX(m_mainBox), m_menuBar, FALSE, FALSE, 0);

    m_toolbar = gtk_ui_manager_get_widget(m_uiManager, "/main-toolbar");
    gtk_box_pack_start(GTK_BOX(m_mainBox), m_toolbar, FALSE, FALSE, 0);

    gtk_box_back_start(GTK_BOX(m_mainBox), m_content.gtkWidget(),
                       TRUE, TRUE, 0);

    g_signal_connect(m_window,
                     "delete-event",
                     G_CALLBACK(onDeleteEvent),
                     this);
    g_signal_connect(m_window,
                     "destroy",
                     G_CALLBACK(onDestroy),
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
    Application::instance().removeWindow(*this);
    delete m_pane;
    if (m_uiManager)
        g_object_unref(m_uiManager);
    if (m_window)
    {
        GtkWidget *w = m_window;
        m_window = NULL;
        gtk_widget_destroy(w);
    }
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
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_NONE,
            _("Closing this window will quit Samoyed."));
        gtk_dialog_add_buttons(
            GTK_DIALOG(dialog),
            _("_Quit"), GTK_RESPONSE_YES,
            _("_Cancel"), GTK_RESPONSE_NO);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response == GTK_RESPONSE_NO)
            return TRUE;

        // Quitting the application will destroy the window if the user doesn't
        // prevent it.
        Application::instance().quit();
        return TRUE;
    }

    // Request to close all editors.  If the user cancels closing an editor,
    // cancel closing this window.
    if (m_pane)
        if (!m_pane->close())
            return TRUE;
    m_closing = true;
    if (m_pane)
        return TRUE;
    return FALSE;
}

void Window::onDestroy(GtkWidget *widget, gpointer window)
{
    Window *w = static_cast<Window *>(window);
    if (w->m_window)
    {
        assert(w->m_window == widget);
        w->m_window = NULL;
        delete w;
    }
}

gboolean Window::onFocusInEvent(GtkWidget *widget,
                                GdkEvent *event,
                                gpointer window)
{
    Application::instance().setCurrentWindow(*static_cast<Window *>(window));
    return FALSE;
}

void Window::setPane(Pane *pane)
{
    // Remove the old pane from this window, which may destroy it.
    if (m_pane)
    {
    }

    // Add the new pane to this window.
    if (pane)
    {
    }

    m_pane = pane;
    if (m_closing && !m_pane)
        delete this;
}

}
