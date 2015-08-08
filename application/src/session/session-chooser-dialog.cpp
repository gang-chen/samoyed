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
            gtk_window_set_modal(GTK_WINDOW(m_newSessionDialog), true);
        }
        m_newSessionNameEntry =
            GTK_ENTRY(gtk_builder_get_object(m_builder,
                                             "new-session-name-entry"));
        g_signal_connect(m_newSessionDialog, "response",
                         G_CALLBACK(onNewSessionResponse), this);
    }
}

void SessionChooserDialog::buildRestoreSessionDialog()
{
    if (!m_restoreSessionDialog)
    {
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

        m_restoreSessionDialog =
            GTK_DIALOG(gtk_builder_get_object(m_builder,
                                              "restore-session-dialog"));
        if (m_parent)
        {
            gtk_window_set_transient_for(GTK_WINDOW(m_restoreSessionDialog),
                                         m_parent);
            gtk_window_set_modal(GTK_WINDOW(m_restoreSessionDialog), true);
        }
        m_sessionList =
            GTK_TREE_VIEW(gtk_builder_get_object(m_builder, "session-list"));
        g_signal_connect(m_restoreSessionDialog, "response",
                         G_CALLBACK(onRestoreSessionResponse), this);
        g_signal_connect(m_sessionList, "row-activated",
                         G_CALLBACK(onSessionListRowActivated), this);
    }
}

void SessionChooserDialog::onNewSessionResponse(GtkDialog *gtkDialog,
                                                gint response,
                                                SessionChooserDialog *dialog)
{
    if (response == GTK_RESPONSE_OK)
    {
        dialog->m_sessionName =
            gtk_entry_get_text(dialog->m_newSessionNameEntry);
        if (dialog->m_sessionName.empty())
        {
            GtkWidget *d = gtk_message_dialog_new(
                GTK_WINDOW(gtkDialog),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("The new session name is empty. Type it in the input box."));
            gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            g_signal_stop_emission_by_name(gtkDialog, "response");
            gtk_widget_grab_focus(GTK_WIDGET(dialog->m_newSessionNameEntry));
        }
    }
}

void
SessionChooserDialog::onRestoreSessionResponse(GtkDialog *gtkDialog,
                                               gint response,
                                               SessionChooserDialog *dialog)
{
    if (response == GTK_RESPONSE_OK)
    {
        GtkTreeSelection *selection;
        GtkTreeModel *model;
        GtkTreeIter it;
        selection = gtk_tree_view_get_selection(dialog->m_sessionList);
        if (!gtk_tree_selection_get_selected(selection, &model, &it))
        {
            GtkWidget *d = gtk_message_dialog_new(
                GTK_WINDOW(gtkDialog),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("No session is chosen. Choose one from the session list."));
            gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            g_signal_stop_emission_by_name(gtkDialog, "response");
            gtk_widget_grab_focus(GTK_WIDGET(dialog->m_sessionList));
        }
        else
        {
            char *sessionName;
            gtk_tree_model_get(model, &it, NAME_COLUMN, &sessionName, -1);
            dialog->m_sessionName = sessionName;
            g_free(sessionName);
        }
    }
}

void
SessionChooserDialog::onSessionListRowActivated(GtkTreeView *list,
                                                GtkTreeIter *iter,
                                                GtkTreePath *path,
                                                SessionChooserDialog *dialog)
{
    gtk_window_activate_default(GTK_WINDOW(dialog->m_restoreSessionDialog));
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
                if (response != GTK_RESPONSE_OK)
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
                if (response != GTK_RESPONSE_OK)
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
