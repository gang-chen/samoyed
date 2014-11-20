// Session management window.
// Copyright (C) 2014 Gang Chen.

/*
UNIT TEST BUILD
g++ session-management-window.cpp -DSMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST\
 -I../.. `pkg-config --cflags --libs gtk+-3.0` -Werror -Wall\
 -o session-management-window
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session-management-window.hpp"
#include "utilities/miscellaneous.hpp"
#ifndef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST
# include "ui/session.hpp"
#endif
#ifdef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST
# include <stdio.h>
#endif
#include <string.h>
#include <list>
#include <string>
#ifdef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST
# define _(T) T
#else
# include <glib/gi18n.h>
#endif
#include <gtk/gtk.h>

#ifdef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST

namespace Samoyed
{

class Session
{
public:
    enum LockState
    {
        STATE_UNLOCKED,
        STATE_LOCKED_BY_THIS_PROCESS,
        STATE_LOCKED_BY_ANOTHER_PROCESS
    };
};

}

#endif

namespace
{

enum Columns
{
    NAME_COLUMN,
    LOCKED_COLUMN,
    N_COLUMNS
};

bool readSessions(std::list<std::string> &sessionNames,
                  std::list<Samoyed::Session::LockState> &sessionStates)
{
#ifdef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST
    sessionNames.push_back("unlocked");
    sessionStates.push_back(Samoyed::Session::STATE_UNLOCKED);
    sessionNames.push_back("locked-by-this");
    sessionStates.push_back(Samoyed::Session::STATE_LOCKED_BY_THIS_PROCESS);
    sessionNames.push_back("locked-by-another");
    sessionStates.push_back(Samoyed::Session::STATE_LOCKED_BY_ANOTHER_PROCESS);
    return true;
#else
    bool successful;
    successful = Samoyed::Session::readAllSessionNames(sessionNames);
    for (std::list<std::string>::const_iterator it = sessionNames.begin();
         it != sessionNames.end();
         ++it)
        sessionStates.push_back(Samoyed::Session::queryLockState(it->c_str()));
    return successful;
#endif
}

void setSessionNameEditable(GtkTreeViewColumn *column,
                            GtkCellRenderer *renderer,
                            GtkTreeModel *model,
                            GtkTreeIter *iter,
                            gpointer data)
{
    gboolean locked;
    gtk_tree_model_get(model, iter, LOCKED_COLUMN, &locked, -1);
    g_object_set(renderer, "editable", !locked, NULL);
}

}

namespace Samoyed
{

void SessionManagementWindow::deleteSelectedSession(bool confirm)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter it;
    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(m_sessionList));

    if (!gtk_tree_selection_get_selected(selection, &model, &it))
    {
        GtkWidget *d = gtk_message_dialog_new(
            GTK_WINDOW(m_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("No session is chosen. Choose the session you want to delete "
              "from the session list."));
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }

    char *sessionName;
    gtk_tree_model_get(model, &it, NAME_COLUMN, &sessionName, -1);

    gboolean locked;
    gtk_tree_model_get(model, &it, LOCKED_COLUMN, &locked, -1);
    if (locked)
    {
        GtkWidget *d = gtk_message_dialog_new(
            GTK_WINDOW(m_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Session \"%s\" can't be deleted because it is locked."),
            sessionName);
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(sessionName);
        return;
    }

    if (confirm)
    {
        GtkWidget *d = gtk_message_dialog_new(
            GTK_WINDOW(m_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("Are you sure to delete session \"%s\"?"),
            sessionName);
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_NO);
        if (gtk_dialog_run(GTK_DIALOG(d)) != GTK_RESPONSE_YES)
        {
            gtk_widget_destroy(d);
            g_free(sessionName);
            return;
        }
        gtk_widget_destroy(d);
    }

#ifdef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST
    printf("Remove session \"%s\".\n", sessionName);
#else
    if (Session::remove(sessionName))
#endif
    gtk_list_store_remove(GTK_LIST_STORE(model), &it);
    g_free(sessionName);
}

void SessionManagementWindow::onDeleteSession(GtkButton *button,
                                              SessionManagementWindow *window)
{
    window->deleteSelectedSession(true);
}

void SessionManagementWindow::onRenameSession(GtkButton *button,
                                              SessionManagementWindow *window)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter it;

    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(window->m_sessionList));

    if (!gtk_tree_selection_get_selected(selection, &model, &it))
    {
        GtkWidget *d = gtk_message_dialog_new(
            GTK_WINDOW(window->m_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("No session is chosen. Choose the session you want to rename "
              "from the session list."));
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }

    gboolean locked;
    gtk_tree_model_get(model, &it, LOCKED_COLUMN, &locked, -1);
    if (locked)
    {
        char *sessionName;
        gtk_tree_model_get(model, &it, NAME_COLUMN, &sessionName, -1);
        GtkWidget *d = gtk_message_dialog_new(
            GTK_WINDOW(window->m_window),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Session \"%s\" can't be renamed because it is locked."),
            sessionName);
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(sessionName);
        return;
    }

    // Start editing.
    GtkTreePath *path;
    GtkTreeViewColumn *column;
    gtk_widget_grab_focus(window->m_sessionList);
    path = gtk_tree_model_get_path(model, &it);
    column = gtk_tree_view_get_column(GTK_TREE_VIEW(window->m_sessionList),
                                      NAME_COLUMN);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(window->m_sessionList),
                             path,
                             column,
                             TRUE);
    gtk_tree_path_free(path);
}

void SessionManagementWindow::onSessionRenamed(GtkCellRendererText *renderer,
                                               char *path,
                                               char *newName,
                                               SessionManagementWindow *window)
{
    GtkTreeModel *model;
    GtkTreePath *p;
    GtkTreeIter it;
    model = gtk_tree_view_get_model(GTK_TREE_VIEW(window->m_sessionList));
    p = gtk_tree_path_new_from_string(path);
    if (gtk_tree_model_get_iter(model, &it, p))
    {
        char *oldName;
        gtk_tree_model_get(model, &it, NAME_COLUMN, &oldName, -1);
        if (strcmp(oldName, newName))
        {
#ifdef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST
            printf("Rename session \"%s\" with \"%s\".\n", oldName, newName);
#else
            if (Session::rename(oldName, newName))
#endif
            gtk_list_store_set(GTK_LIST_STORE(model), &it,
                               NAME_COLUMN, newName, -1);
        }
        g_free(oldName);
    }
    gtk_tree_path_free(p);
}

gboolean SessionManagementWindow::onKeyPress(GtkWidget *widget,
                                             GdkEventKey *event,
                                             SessionManagementWindow *window)
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    if (event->keyval == GDK_KEY_Delete || event->keyval == GDK_KEY_KP_Delete)
    {
        if ((event->state & modifiers) == GDK_SHIFT_MASK)
            window->deleteSelectedSession(false);
        else
            window->deleteSelectedSession(true);
        return TRUE;
    }
    return FALSE;
}

void
SessionManagementWindow::onDestroyInternally(GtkWidget *widget,
                                             SessionManagementWindow *window)
{
    window->m_window = NULL;
    if (window->m_onDestroy)
        window->m_onDestroy(*window);
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
    GtkListStore *store = gtk_list_store_new(N_COLUMNS,
                                             G_TYPE_STRING, G_TYPE_BOOLEAN);
    GtkTreeIter it;
    std::list<std::string>::const_iterator it1 = sessionNames.begin();
    std::list<Samoyed::Session::LockState>::const_iterator it2 =
        sessionStates.begin();
    for (; it1 != sessionNames.end(); ++it1, ++it2)
    {
        gtk_list_store_append(store, &it);
        gtk_list_store_set(store, &it,
                           NAME_COLUMN, it1->c_str(),
                           LOCKED_COLUMN,
                           *it2 != Samoyed::Session::STATE_UNLOCKED,
                           -1);
    }

    // Make the session list.
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    m_sessionList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    renderer = gtk_cell_renderer_text_new();
    g_signal_connect(renderer, "edited", G_CALLBACK(onSessionRenamed), this);
    column = gtk_tree_view_column_new_with_attributes(_("Name"),
                                                      renderer,
                                                      "text", NAME_COLUMN,
                                                      NULL);
    gtk_tree_view_column_set_cell_data_func(column, renderer,
                                            setSessionNameEditable, NULL, NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_sessionList), column);
    renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_activatable(
        GTK_CELL_RENDERER_TOGGLE(renderer), FALSE);
    column = gtk_tree_view_column_new_with_attributes(_("Locked"),
                                                      renderer,
                                                      "active", LOCKED_COLUMN,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_sessionList), column);
    gtk_widget_set_hexpand(m_sessionList, TRUE);
    gtk_widget_set_vexpand(m_sessionList, TRUE);
    g_signal_connect(m_sessionList, "key-press-event",
                     G_CALLBACK(onKeyPress), this);

    GtkWidget *deleteButton = gtk_button_new_with_mnemonic(_("_Delete"));
    g_signal_connect(deleteButton, "clicked",
                     G_CALLBACK(onDeleteSession), this);
    GtkWidget *renameButton = gtk_button_new_with_mnemonic(_("_Rename..."));
    g_signal_connect(renameButton, "clicked",
                     G_CALLBACK(onRenameSession), this);

    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(grid), m_sessionList, 0, 0, 1, 1);
    GtkWidget *buttons = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(buttons), deleteButton, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttons), renameButton, FALSE, TRUE, 0);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(buttons), GTK_BUTTONBOX_SPREAD);
    gtk_box_set_spacing(GTK_BOX(buttons), CONTAINER_SPACING);
    gtk_grid_attach(GTK_GRID(grid), buttons, 0, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(m_window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), CONTAINER_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(grid), CONTAINER_BORDER_WIDTH);

    gtk_window_set_title(GTK_WINDOW(m_window), _("Sessions"));
    if (parent)
    {
        gtk_window_set_transient_for(GTK_WINDOW(m_window), parent);
        gtk_window_set_destroy_with_parent(GTK_WINDOW(m_window), TRUE);
    }
    g_signal_connect(m_window, "destroy",
                     G_CALLBACK(onDestroyInternally), this);
    gtk_widget_show_all(grid);
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

#ifdef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST

namespace
{

void deleteWindow(Samoyed::SessionManagementWindow &window)
{
    delete &window;
    gtk_main_quit();
}

}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    Samoyed::SessionManagementWindow *window1, *window2;
    window1 = new Samoyed::SessionManagementWindow(NULL, NULL);
    window1->show();
    window2 = new Samoyed::SessionManagementWindow(NULL, deleteWindow);
    window2->show();
    gtk_main();
    return 0;
}

#endif
