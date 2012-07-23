// Top-level window.
// Copyright (C) 2011 Gang Chen.

#include "window.hpp"
#include "actions.hpp"
#include <assert.h>
#include <gtk/gtk.h>

namespace Samoyed
{

Window::Window()
{
}

bool Window::create(const Configuration *config)
{
    uiManager = gtk_ui_manager_new();

    m_basicActions = gtk_action_group_new("basic actions");
    gtk_action_group_set_translation_domain(m_basicActions, NULL);
    gtk_action_group_add_actions(m_basicActions,
                                 Actions::s_basicActionEntries,
                                 G_N_ELEMENTS(Actions::s_basicActionEntries),
                                 this);
    gtk_ui_manager_insert_action_group(m_uiManager, m_basicActions, 0);
    g_object_unref(m_basicActions);

    GError *error = NULL;
    std::string uiFile(Application::instance()->dataDirectory());
    uiFile += G_DIR_SEPARATOR_S "actions-ui.xml";
    gtk_ui_manager_add_ui_from_file(m_uiManager, uiFile.c_str(), &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to load UI file '%s' to construct the user "
              "interface: %s."),
            uiFile.c_str(), error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        g_object_unref(m_uiManager);
        return false;
    }

    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    m_mainBox = gtk_vbox_new();
    gtk_container_add(GTK_CONTAINER(m_window), m_mainBox);

    m_menuBar = gtk_ui_manager_get_widget(m_uiManager, "/main-menu-bar");
    gtk_box_pack_start(GTK_BOX(m_mainBox), m_menuBar, false, false, 0);

    m_toolbar = gtk_ui_manager_get_widget(m_uiManager, "/main-toolbar");
    gtk_box_pack_start(GTK_BOX(m_mainBox), m_toolbar, false, false, 0);

    g_signal_connect(m_window,
                     "destroy-event",
                     G_CALLBACK(requstDestroy),
                     this);
    g_signal_connect(m_window,
                     "destroy",
                     G_CALLBACK(onDestroyed),
                     this);
    g_signal_connect(m_window,
                     "focus-in-event",
                     G_CALLBACK(onFocusIn),
                     this);
    return true;
}

Window::~Window()
{
    assert(!m_window);
}

bool Window::destroy()
{
    // Close all the editors in this window.

    // Destroy the window.
    gtk_widget_destroy(m_window);
}

gboolean Window::onDestroyEvent(GtkWidget *widget,
                                GdkEvent *event,
                                gpointer window)
{
    Window *w = static_cast<Window *>(window);
    Application::instance()->destroyWindow(w);
    return TRUE;
}

void Window::onDestroyed(GtkWidget *widget, gpointer window)
{
    Window *w = static_cast<Window *>(window);
    w->m_window = NULL;
    Application::instance()->onWindowDestroyed(*w);
}

gboolean Window::onFocusIn(GtkWidget *widget,
                           GdkEvent *event,
                           gpointer window)
{
    Window *w = static_cast<Window *>(window);
    Application::instance()->onWindowFocusIn(w);
    return FALSE;
}

}
