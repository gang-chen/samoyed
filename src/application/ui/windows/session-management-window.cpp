// Session management window.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session-management-window.hpp"
#include "ui/session.hpp"
#include <list>
#include <string>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

namespace
{

bool readSessions(std::list<std::string> &sessionNames,
                  std::list<Samoyed::Session::LockState> &sessionStates)
{
    bool successful;
    successful = Samoyed::Session::readAllSessionNames(sessionNames);
    for (std::list<std::string>::const_iterator it = sessionNames.begin();
         it != sessionNames.end();
         ++it)
        sessionStates.push_back(Samoyed::Session::queryLockState(it->c_str()));
    return successful;
}

}

namespace Samoyed
{

const char *SessionManagementWindow::sessionName() const
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter it;
    char *sessionName;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_sessions));
    if (!gtk_tree_selection_get_selected(selection, &model, &it))
        return NULL;
    gtk_tree_model_get(model, &it, 0, &sessionName, -1);
    return sessionName;
}

void SessionManagementWindow::onDeleteSession(GtkButton *button,
                                              SessionManagementWindow *window)
{
    const char *name = window->sessionName();
    Session::remove(name);
}

void SessionManagementWindow::onRenameSession(GtkButton *button,
                                              SessionManagementWindow *window)
{
    const char *name = window->sessionName();
    Session::rename(name, newName);
}

void SessionManagementWindow::onDestroy(GtkWidget *widget,
                                        SessionManagementWindow *window)
{
    window->m_window = NULL;
    if (window->m_onDestroy)
        window->m_onDestroy(*this);
    else
        delete window;
}

SessionManagementWindow::SessionManagementWindow(
    GtkWindow *parent,
    const boost::function<void (SessionManagementWindow &)> &onDestroy):
        m_onDestroy(onDestroy)
{
    // Read sessions.
    std::list<std::string> sessionNames;
    std::list<Samoyed::Session::LockState> sessionStates;
    readSessions(sessionNames, sessionStates);
    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
    GtkTreeIter it;
    std::list<std::string>::const_iterator it1 = sessionNames.begin();
    std::list<Samoyed::Session::LockState>::const_iterator it2 =
        sessionStates.begin();
    for (; it1 != sessionNames.end(); ++it1, ++it2)
    {
        gtk_list_store_append(store, &it);
        gtk_list_store_set(store, &it,
                           0, it1->c_str(),
                           1,
                           *it2 != Samoyed::Session::STATE_UNLOCKED,
                           -1);
    }

    // Make the session list.
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    m_sessions = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Name"),
                                                      renderer,
                                                      "text", 0,
                                                      NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_sessions), column);
    renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_activatable(
        GTK_CELL_RENDERER_TOGGLE(renderer), FALSE);
    column = gtk_tree_view_column_new_with_attributes(_("Locked"),
                                                      renderer,
                                                      "active", 1,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_sessions), column);
    gtk_widget_set_hexpand(m_sessions, TRUE);

    GtkWidget *deleteButton = gtk_button_new_with_mnemonic(_("_Delete"));
    g_signal_connect(deleteButton, "clicked",
                     G_CALLBACK(onDeleteSession), this);
    GtkWidget *renameButton = gtk_button_new_with_mnemonic(_("_Rename..."));
    g_signal_connect(renameButton, "clicked",
                     G_CALLBACK(onRenameSession), this);

    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(grid), m_sessions, 0, 0, 1, 1);
    GtkWidget *buttons = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(buttons), deleteButton, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(buttons), renameButton, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), buttons, 0, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(m_window), grid);
    gtk_window_set_title(GTK_WINDOW(m_window), _("Sessions"));
    gtk_window_set_transient_for(GTK_WINDOW(m_window), parent);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(m_window), TRUE);
    g_signal_connect(m_window, "destroy",
                     G_CALLBACK(onDestroy), this);
    gtk_widget_show_all(notebook);
}

SessionManagementWindow::~SessionManagementWindow()
{
    if (m_window)
        gtk_widget_destroy(m_window);
}

void SessionManagementWindow::show()
{
    gtk_widget_show(m_window);
}

void SessionManagementWindow::hide()
{
    gtk_widget_hide(m_window);
}

}
