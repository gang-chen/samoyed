// Top-level window.
// Copyright (C) 2011 Gang Chen.

#include "window.hpp"
#include "actions.hpp"
#include <assert.h>
#include <gtk/gtk.h>

namespace Samoyed
{

Window::Window():
    m_window(NULL),
    m_mainBox(NULL),
    m_menuBar(NULL),
    m_toolbar(NULL),
    m_uiManager(NULL),
    m_basicActions(NULL),
    m_actionsForProjects(NULL),
    m_actionsForFiles(NULL)
{
}

Window *Window::create(const Configuration *config)
{
    Window *w = new Window;

    w->m_uiManager = gtk_ui_manager_new();

    w->m_basicActions = gtk_action_group_new("basic actions");
    gtk_action_group_set_translation_domain(w->m_basicActions, NULL);
    gtk_action_group_add_actions(w->m_basicActions,
                                 Actions::s_basicActionEntries,
                                 G_N_ELEMENTS(Actions::s_basicActionEntries),
                                 w);
    gtk_ui_manager_insert_action_group(w->m_uiManager, w->m_basicActions, 0);
    g_object_unref(w->m_basicActions);

    GError *error = NULL;
    std::string uiFile(Application::instance()->dataDirectory());
    uiFile += G_DIR_SEPARATOR_S "actions-ui.xml";
    gtk_ui_manager_add_ui_from_file(w->m_uiManager, uiFile.c_str(), &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to load UI file '%s' to construct the user "
              "interface: '%s'."),
            uiFile.c_str(), error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        g_object_unref(m_uiManager);
        delete w;
        return NULL;
    }

    w->m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    w->m_mainBox = gtk_vbox_new();
    gtk_container_add(GTK_CONTAINER(w->m_window), w->m_mainBox);

    w->m_menuBar = gtk_ui_manager_get_widget(w->m_uiManager, "/main-menu-bar");
    gtk_box_pack_start(GTK_BOX(w->m_mainBox), w->m_menuBar, false, false, 0);

    w->m_toolbar = gtk_ui_manager_get_widget(w->m_uiManager, "/main-toolbar");
    gtk_box_pack_start(GTK_BOX(w->m_mainBox), w->m_toolbar, false, false, 0);

    g_signal_connect(w->m_window,
                     "delete-event",
                     G_CALLBACK(onDeleteEvent),
                     NULL);

    gtk_widget_show(w->m_window);

    return w;
}

Window::~Window()
{
    if (m_uiManager)
        g_object_unref(m_uiManager);
    if (m_window)
        gtk_widget_destroy(m_window);
}

gboolean Window::onDeleteEvent(GtkWidget *widget,
                               GdkEvent *event,
                               gpointer)
{
    // Quiting the application will destroy the window if the user doesn't
    // prevent it.
    Application::instance()->quit();
    return TRUE;
}

}
