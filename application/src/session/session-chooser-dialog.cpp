// Session chooser dialog.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ session-chooser-dialog.cpp -DSMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST -I.. \
`pkg-config --cflags --libs gtk+-3.0` -Werror -Wall -o session-chooser-dialog
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session-chooser-dialog.hpp"
#include "utilities/miscellaneous.hpp"
#ifndef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST
# include "session.hpp"
# include "application.hpp"
#else
# include <stdio.h>
#endif
#include <list>
#include <string>
#ifdef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST
# define _(T) T
#else
# include <glib/gi18n.h>
#endif
#include <gtk/gtk.h>

#ifdef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST

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

enum Response
{
    RESPONSE_SWITCH_TO_RESTORE_SESSION = 1,
    RESPONSE_SWITCH_TO_NEW_SESSION = 2
};

enum Columns
{
    NAME_COLUMN,
    LOCKED_COLUMN,
    N_COLUMNS
};

bool readSessions(std::list<std::string> &sessionNames,
                  std::list<Samoyed::Session::LockState> &sessionStates)
{
#ifdef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST
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

void onNewSessionChanged(GtkEditable *editable, GtkDialog *dialog)
{
    gtk_widget_set_sensitive(
        gtk_dialog_get_widget_for_response(dialog, GTK_RESPONSE_ACCEPT),
        *gtk_editable_get_chars(editable, 0, -1));
}

void onRestoreSessionChanged(GtkTreeSelection *selection, GtkDialog *dialog)
{
    gtk_widget_set_sensitive(
        gtk_dialog_get_widget_for_response(dialog, GTK_RESPONSE_ACCEPT),
        gtk_tree_selection_get_selected(selection, NULL, NULL));
}

void onSessionListRowActivated(GtkTreeView *list,
                               GtkTreeIter *iter,
                               GtkTreePath *path,
                               GtkWindow *dialog)
{
    gtk_window_activate_default(dialog);
}

}

namespace Samoyed
{

void SessionChooserDialog::buildNewSessionDialog()
{
    if (!m_newSessionDialog)
    {
        m_newSessionDialog =
            GTK_DIALOG(gtk_builder_get_object(m_builder, "new-session-dialog"));
        if (m_parent)
        {
            gtk_window_set_transient_for(GTK_WINDOW(m_newSessionDialog),
                                         m_parent);
            gtk_window_set_modal(GTK_WINDOW(m_newSessionDialog), TRUE);
        }
        m_newSessionNameEntry =
            GTK_ENTRY(gtk_builder_get_object(m_builder,
                                             "new-session-name-entry"));
        g_signal_connect(GTK_EDITABLE(m_newSessionNameEntry), "changed",
                         G_CALLBACK(onNewSessionChanged), m_newSessionDialog);
        g_signal_connect(GTK_WIDGET(m_newSessionDialog), "delete-event",
                         G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    }
}

void SessionChooserDialog::buildRestoreSessionDialog()
{
    // Read sessions.
    std::list<std::string> sessionNames;
    std::list<Samoyed::Session::LockState> sessionStates;
    readSessions(sessionNames, sessionStates);
    GtkListStore *store =
        GTK_LIST_STORE(gtk_builder_get_object(m_builder, "sessions"));
    gtk_list_store_clear(store);
    GtkTreeIter it;
    std::list<std::string>::const_iterator it1 = sessionNames.begin();
    std::list<Samoyed::Session::LockState>::const_iterator it2 =
        sessionStates.begin();
    for (; it1 != sessionNames.end(); ++it1, ++it2)
    {
        if (*it2 == Samoyed::Session::STATE_UNLOCKED)
        {
            gtk_list_store_append(store, &it);
            gtk_list_store_set(store, &it,
                               NAME_COLUMN, it1->c_str(),
                               -1);
        }
    }

    if (!m_restoreSessionDialog)
    {
        m_restoreSessionDialog =
            GTK_DIALOG(gtk_builder_get_object(m_builder,
                                              "restore-session-dialog"));
        if (m_parent)
        {
            gtk_window_set_transient_for(GTK_WINDOW(m_restoreSessionDialog),
                                         m_parent);
            gtk_window_set_modal(GTK_WINDOW(m_restoreSessionDialog), TRUE);
        }
        m_sessionList =
            GTK_TREE_VIEW(gtk_builder_get_object(m_builder, "session-list"));
        gtk_widget_set_sensitive(
            GTK_WIDGET(gtk_builder_get_object(m_builder,
                                              "restore-session-button")),
            gtk_tree_selection_get_selected(
                gtk_tree_view_get_selection(m_sessionList), NULL, NULL));
        g_signal_connect(gtk_tree_view_get_selection(m_sessionList),
                         "changed",
                         G_CALLBACK(onRestoreSessionChanged),
                         m_restoreSessionDialog);
        g_signal_connect(m_sessionList, "row-activated",
                         G_CALLBACK(onSessionListRowActivated),
                         m_restoreSessionDialog);
        g_signal_connect(GTK_WIDGET(m_restoreSessionDialog), "delete-event",
                         G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    }
}

SessionChooserDialog::SessionChooserDialog(Action action, GtkWindow *parent):
    m_action(action),
    m_parent(parent),
    m_newSessionDialog(NULL),
    m_restoreSessionDialog(NULL)
{
#ifdef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST
    std::string uiFile(".." G_DIR_SEPARATOR_S ".." G_DIR_SEPARATOR_S "data");
#else
    std::string uiFile(Application::instance().dataDirectoryName());
#endif
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "session-chooser-dialog.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());
}

SessionChooserDialog::~SessionChooserDialog()
{
    gtk_widget_destroy(GTK_WIDGET(
        gtk_builder_get_object(m_builder, "new-session-dialog")));
    gtk_widget_destroy(GTK_WIDGET(
        gtk_builder_get_object(m_builder, "restore-session-dialog")));
    g_object_unref(m_builder);
}

void SessionChooserDialog::run()
{
    while (m_action != ACTION_CANCEL)
    {
        if (m_action == ACTION_CREATE)
        {
            buildNewSessionDialog();
            int response = gtk_dialog_run(m_newSessionDialog);
            gtk_widget_hide(GTK_WIDGET(m_newSessionDialog));
            if (response == RESPONSE_SWITCH_TO_RESTORE_SESSION)
                m_action = ACTION_RESTORE;
            else
            {
                if (response == GTK_RESPONSE_ACCEPT)
                    m_sessionName = gtk_entry_get_text(m_newSessionNameEntry);
                else
                    m_action = ACTION_CANCEL;
                break;
            }
        }
        else
        {
            buildRestoreSessionDialog();
            int response = gtk_dialog_run(m_restoreSessionDialog);
            gtk_widget_hide(GTK_WIDGET(m_restoreSessionDialog));
            if (response == RESPONSE_SWITCH_TO_NEW_SESSION)
                m_action = ACTION_CREATE;
            else
            {
                if (response == GTK_RESPONSE_ACCEPT)
                {
                    GtkTreeSelection *selection;
                    GtkTreeModel *model;
                    GtkTreeIter it;
                    char *sessionName;
                    selection = gtk_tree_view_get_selection(m_sessionList);
                    gtk_tree_selection_get_selected(selection, &model, &it);
                    gtk_tree_model_get(model, &it, NAME_COLUMN, &sessionName,
                                       -1);
                    m_sessionName = sessionName;
                    g_free(sessionName);
                }
                else
                    m_action = ACTION_CANCEL;
                break;
            }
        }
    }
}

}

#ifdef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    Samoyed::SessionChooserDialog *dialog = new Samoyed::SessionChooserDialog(
        Samoyed::SessionChooserDialog::ACTION_CREATE,
        NULL);
    dialog->run();
    if (dialog->action() == Samoyed::SessionChooserDialog::ACTION_CANCEL)
        printf("Canceled.\n");
    else if (dialog->action() == Samoyed::SessionChooserDialog::ACTION_CREATE)
        printf("Create session \"%s\".\n", dialog->sessionName());
    else if (dialog->action() == Samoyed::SessionChooserDialog::ACTION_RESTORE)
        printf("Restore session \"%s\".\n", dialog->sessionName());
    delete dialog;
    dialog = new Samoyed::SessionChooserDialog(
        Samoyed::SessionChooserDialog::ACTION_RESTORE,
        NULL);
    dialog->run();
    if (dialog->action() == Samoyed::SessionChooserDialog::ACTION_CANCEL)
        printf("Canceled.\n");
    else if (dialog->action() == Samoyed::SessionChooserDialog::ACTION_CREATE)
        printf("Create session \"%s\".\n", dialog->sessionName());
    else if (dialog->action() == Samoyed::SessionChooserDialog::ACTION_RESTORE)
        printf("Restore session \"%s\".\n", dialog->sessionName());
    delete dialog;
    return 0;
}

#endif
