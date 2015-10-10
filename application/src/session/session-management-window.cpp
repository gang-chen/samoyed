// Session management window.
// Copyright (C) 2014 Gang Chen.

/*
UNIT TEST BUILD
g++ session-management-window.cpp -DSMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST \
-I.. `pkg-config --cflags --libs gtk+-3.0` -Werror -Wall \
-o session-management-window
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session-management-window.hpp"
#include "utilities/miscellaneous.hpp"
#ifndef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST
# include "session.hpp"
# include "application.hpp"
#else
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

gboolean onDeleteEvent(GtkWidget *widget,
                       GdkEvent *event,
                       Samoyed::SessionManagementWindow *window)
{
    delete window;
    return TRUE;
}

}

namespace Samoyed
{

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
            window->m_window,
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
            window->m_window,
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
    gtk_widget_grab_focus(GTK_WIDGET(window->m_sessionList));
    path = gtk_tree_model_get_path(model, &it);
    column = gtk_tree_view_get_column(GTK_TREE_VIEW(window->m_sessionList),
                                      NAME_COLUMN);
    gtk_tree_view_set_cursor(GTK_TREE_VIEW(window->m_sessionList),
                             path,
                             column,
                             TRUE);
    gtk_tree_path_free(path);
}

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
            m_window,
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
            m_window,
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
            m_window,
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

void SessionManagementWindow::onSessionSelectionChanged(
    GtkTreeSelection *selection,
    SessionManagementWindow *window)
{
    gboolean sensitive;
    GtkTreeModel *model;
    GtkTreeIter it;
    gboolean locked;

    if (!gtk_tree_selection_get_selected(selection, &model, &it))
        sensitive = false;
    else
    {
        gtk_tree_model_get(model, &it, LOCKED_COLUMN, &locked, -1);
        sensitive = !locked;
    }
    gtk_widget_set_sensitive(
        GTK_WIDGET(gtk_builder_get_object(window->m_builder,
                                          "rename-session-button")),
        sensitive);
    gtk_widget_set_sensitive(
        GTK_WIDGET(gtk_builder_get_object(window->m_builder,
                                          "delete-session-button")),
        sensitive);
}

SessionManagementWindow::SessionManagementWindow(
    GtkWindow *parent,
    const boost::function<void (SessionManagementWindow &)> &onDestroyed):
        m_onDestroyed(onDestroyed)
{
#ifdef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST
    std::string uiFile(".." G_DIR_SEPARATOR_S ".." G_DIR_SEPARATOR_S "data");
#else
    std::string uiFile(Application::instance().dataDirectoryName());
#endif
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "session-management-window.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());

    // Read sessions.
    std::list<std::string> sessionNames;
    std::list<Samoyed::Session::LockState> sessionStates;
    readSessions(sessionNames, sessionStates);
    GtkListStore *store =
        GTK_LIST_STORE(gtk_builder_get_object(m_builder, "sessions"));
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

    m_sessionList =
        GTK_TREE_VIEW(gtk_builder_get_object(m_builder, "session-list"));
    gtk_tree_view_column_set_cell_data_func(
        GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(m_builder,
                                                    "session-name-column")),
        GTK_CELL_RENDERER(gtk_builder_get_object(m_builder,
                                                 "session-name-renderer")),
        setSessionNameEditable, NULL, NULL);
    g_signal_connect(m_sessionList, "key-press-event",
                     G_CALLBACK(onKeyPress), this);
    GList *cells = gtk_cell_layout_get_cells(
        GTK_CELL_LAYOUT(gtk_tree_view_get_column(m_sessionList, 0)));
    g_signal_connect(GTK_CELL_RENDERER_TEXT(cells->data), "edited",
                     G_CALLBACK(onSessionRenamed), this);
    g_list_free(cells);
    g_signal_connect(gtk_tree_view_get_selection(m_sessionList), "changed",
                     G_CALLBACK(onSessionSelectionChanged), this);

    g_signal_connect(gtk_builder_get_object(m_builder, "rename-session-button"),
                     "clicked",
                     G_CALLBACK(onRenameSession), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "delete-session-button"),
                     "clicked",
                     G_CALLBACK(onDeleteSession), this);

    m_window = GTK_WINDOW(gtk_builder_get_object(m_builder,
                                                 "session-management-window"));
    if (parent)
    {
        gtk_window_set_transient_for(m_window, parent);
        gtk_window_set_destroy_with_parent(m_window, TRUE);
    }
    g_signal_connect(m_window, "delete-event",
                     G_CALLBACK(onDeleteEvent), this);
}

SessionManagementWindow::~SessionManagementWindow()
{
    gtk_widget_destroy(GTK_WIDGET(m_window));
    g_object_unref(m_builder);
    if (m_onDestroyed)
        m_onDestroyed(*this);
}

void SessionManagementWindow::show()
{
    gtk_widget_show(GTK_WIDGET(m_window));
}

void SessionManagementWindow::hide()
{
    gtk_widget_hide(GTK_WIDGET(m_window));
}

}

#ifdef SMYD_SESSION_MANAGEMENT_WINDOW_UNIT_TEST

namespace
{

void onWindowDestroyed(Samoyed::SessionManagementWindow &window)
{
    gtk_main_quit();
}

}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    Samoyed::SessionManagementWindow *window1, *window2;
    window1 = new Samoyed::SessionManagementWindow(NULL, NULL);
    window1->show();
    window2 = new Samoyed::SessionManagementWindow(NULL, onWindowDestroyed);
    window2->show();
    gtk_main();
    return 0;
}

#endif
