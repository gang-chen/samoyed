// Session chooser dialog.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ session-chooser-dialog.cpp ../../utilities/miscellaneous.cpp\
 -DSMYD_UNIT_TEST -DSMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST\
 `pkg-config --cflags --libs gtk+-3.0` -Werror -Wall -o session-chooser-dialog
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session-chooser-dialog.hpp"
#ifndef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST
# include "ui/session.hpp"
#endif
#include "utilities/miscellaneous.hpp"
#ifdef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST
# include <stdio.h>
#endif
#include <string>
#include <list>
#ifdef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST
# define _(T) T
#else
# include <glib/gi18n-lib.h>
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
    RESPONSE_NEW_SESSION = 1,
    RESPONSE_RESTORE_SESSION,
};

class NewSessionDialog
{
public:
    NewSessionDialog(GtkWindow *parent);
    ~NewSessionDialog();
    int run();
    const char *name() const;

private:
    static void onResponse(GtkDialog *gtkDialog,
                           gint response,
                           NewSessionDialog *dialog);
    GtkWidget *m_name;
    GtkWidget *m_dialog;
};

class RestoreSessionDialog
{
public:
    RestoreSessionDialog(GtkWindow *parent);
    ~RestoreSessionDialog();
    int run();
    const char *name() const;

private:
    static void onResponse(GtkDialog *gtkDialog,
                           gint response,
                           RestoreSessionDialog *dialog);
    static void onRowActivated(GtkTreeView *list,
                               GtkTreeIter *iter,
                               GtkTreePath *path,
                               RestoreSessionDialog *dialog);
    GtkListStore *m_store;
    GtkWidget *m_list;
    GtkWidget *m_dialog;
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

void NewSessionDialog::onResponse(GtkDialog *gtkDialog,
                                  gint response,
                                  NewSessionDialog *dialog)
{
    if (response == GTK_RESPONSE_OK)
    {
        const char *sessionName = dialog->name();
        if (!*sessionName)
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
            gtk_widget_grab_focus(dialog->m_name);
        }
        if (!Samoyed::isValidFileName(sessionName))
        {
            GtkWidget *d = gtk_message_dialog_new(
                GTK_WINDOW(gtkDialog),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("The new session name \"%s\" is invalid. Correct it."),
                sessionName);
            Samoyed::gtkMessageDialogAddDetails(
                d,
                _("Only letters, digits, \"_\", \"-\", \"+\" and \".\" are "
                  "allowed in session names."));
            gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
            gtk_dialog_run(GTK_DIALOG(d));
            gtk_widget_destroy(d);
            g_signal_stop_emission_by_name(gtkDialog, "response");
            gtk_widget_grab_focus(dialog->m_name);
        }
    }
}

NewSessionDialog::NewSessionDialog(GtkWindow *parent)
{
    GtkWidget *label = gtk_label_new_with_mnemonic(_("_Session name:"));
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    m_name = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(m_name), "default");
    gtk_entry_set_activates_default(GTK_ENTRY(m_name), TRUE);
    gtk_widget_set_hexpand(m_name, TRUE);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), m_name);
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            label, NULL,
                            GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            m_name, label,
                            GTK_POS_RIGHT, 1, 1);
    gtk_grid_set_column_spacing(GTK_GRID(grid), Samoyed::CONTAINER_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(grid),
                                   Samoyed::CONTAINER_BORDER_WIDTH);
    m_dialog = gtk_dialog_new_with_buttons(
        _("New Session"),
        parent,
        GTK_DIALOG_MODAL,
        GTK_STOCK_OK, GTK_RESPONSE_OK,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        _("_Restore Session..."), RESPONSE_RESTORE_SESSION,
        NULL);
    gtk_container_add(
        GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(m_dialog))),
        grid);
    gtk_widget_set_tooltip_text(
        gtk_dialog_get_widget_for_response(GTK_DIALOG(m_dialog),
                                           RESPONSE_RESTORE_SESSION),
        _("Restore a saved session instead of creating a new one"));
    gtk_button_box_set_child_secondary(
        GTK_BUTTON_BOX(gtk_dialog_get_action_area(GTK_DIALOG(m_dialog))),
        gtk_dialog_get_widget_for_response(GTK_DIALOG(m_dialog),
                                           RESPONSE_RESTORE_SESSION),
        TRUE);
    gtk_button_box_set_child_non_homogeneous(
        GTK_BUTTON_BOX(gtk_dialog_get_action_area(GTK_DIALOG(m_dialog))),
        gtk_dialog_get_widget_for_response(GTK_DIALOG(m_dialog),
                                           RESPONSE_RESTORE_SESSION),
        TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(m_dialog), GTK_RESPONSE_OK);
    g_signal_connect(m_dialog, "response", G_CALLBACK(onResponse), this);
    gtk_widget_show_all(grid);
}

NewSessionDialog::~NewSessionDialog()
{
    gtk_widget_destroy(m_dialog);
}

int NewSessionDialog::run()
{
    return gtk_dialog_run(GTK_DIALOG(m_dialog));
}

const char *NewSessionDialog::name() const
{
    return gtk_entry_get_text(GTK_ENTRY(m_name));
}

void
RestoreSessionDialog::onResponse(GtkDialog *gtkDialog,
                                 gint response,
                                 RestoreSessionDialog *dialog)
{
    if (response == GTK_RESPONSE_OK)
    {
        const char *sessionName = dialog->name();
        if (!sessionName)
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
            gtk_widget_grab_focus(dialog->m_list);
        }
    }
}

void
RestoreSessionDialog::onRowActivated(GtkTreeView *list,
                                     GtkTreeIter *iter,
                                     GtkTreePath *path,
                                     RestoreSessionDialog *dialog)
{
    gtk_window_activate_default(GTK_WINDOW(dialog->m_dialog));
}

RestoreSessionDialog::RestoreSessionDialog(GtkWindow *parent):
    m_store(NULL),
    m_list(NULL),
    m_dialog(NULL)
{
    // Read sessions.
    std::list<std::string> sessionNames;
    std::list<Samoyed::Session::LockState> sessionStates;
    readSessions(sessionNames, sessionStates);
    if (sessionNames.empty())
        return;
    m_store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
    GtkTreeIter it;
    std::list<std::string>::const_iterator it1 = sessionNames.begin();
    std::list<Samoyed::Session::LockState>::const_iterator it2 =
        sessionStates.begin();
    for (; it1 != sessionNames.end(); ++it1, ++it2)
    {
        gtk_list_store_append(m_store, &it);
        gtk_list_store_set(m_store, &it,
                           0, it1->c_str(),
                           1,
                           *it2 != Samoyed::Session::STATE_UNLOCKED,
                           -1);
    }

    // Make the session list.
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    m_list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_store));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("Name"),
                                                      renderer,
                                                      "text", 0,
                                                      NULL);
    gtk_tree_view_column_set_expand(column, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_list), column);
    renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_activatable(
        GTK_CELL_RENDERER_TOGGLE(renderer), FALSE);
    column = gtk_tree_view_column_new_with_attributes(_("Locked"),
                                                      renderer,
                                                      "active", 1,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_list), column);
    gtk_widget_set_hexpand(m_list, TRUE);

    // Make the dialog.
    GtkWidget *label = gtk_label_new_with_mnemonic(_("Choose a _session:"));
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), m_list);
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            label, NULL,
                            GTK_POS_BOTTOM, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            m_list, label,
                            GTK_POS_BOTTOM, 1, 1);
    gtk_grid_set_row_spacing(GTK_GRID(grid), Samoyed::CONTAINER_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(grid),
                                   Samoyed::CONTAINER_BORDER_WIDTH);
    m_dialog = gtk_dialog_new_with_buttons(
        _("Restore Session"),
        parent,
        GTK_DIALOG_MODAL,
        GTK_STOCK_OK, GTK_RESPONSE_OK,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        _("_New Session..."), RESPONSE_NEW_SESSION,
        NULL);
    gtk_container_add(
        GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(m_dialog))),
        grid);
    gtk_widget_set_tooltip_text(
        gtk_dialog_get_widget_for_response(GTK_DIALOG(m_dialog),
                                           RESPONSE_NEW_SESSION),
        _("Create a session instead of restoring a saved one"));
    gtk_button_box_set_child_secondary(
        GTK_BUTTON_BOX(gtk_dialog_get_action_area(GTK_DIALOG(m_dialog))),
        gtk_dialog_get_widget_for_response(GTK_DIALOG(m_dialog),
                                           RESPONSE_NEW_SESSION),
        TRUE);
    gtk_button_box_set_child_non_homogeneous(
        GTK_BUTTON_BOX(gtk_dialog_get_action_area(GTK_DIALOG(m_dialog))),
        gtk_dialog_get_widget_for_response(GTK_DIALOG(m_dialog),
                                           RESPONSE_NEW_SESSION),
        TRUE);
    gtk_dialog_set_default_response(GTK_DIALOG(m_dialog), GTK_RESPONSE_OK);
    g_signal_connect(m_dialog, "response", G_CALLBACK(onResponse), this);
    g_signal_connect(m_list, "row-activated", G_CALLBACK(onRowActivated), this);
    gtk_widget_show_all(grid);
}

RestoreSessionDialog::~RestoreSessionDialog()
{
    if (m_dialog)
    {
        gtk_widget_destroy(m_dialog);
        g_object_unref(m_store);
    }
}

int RestoreSessionDialog::run()
{
    if (!m_dialog)
        return RESPONSE_NEW_SESSION;
    return gtk_dialog_run(GTK_DIALOG(m_dialog));
}

const char *RestoreSessionDialog::name() const
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter it;
    char *sessionName;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(m_list));
    if (!gtk_tree_selection_get_selected(selection, &model, &it))
        return NULL;
    gtk_tree_model_get(model, &it, 0, &sessionName, -1);
    return sessionName;
}

}

namespace Samoyed
{

void SessionChooserDialog::run()
{
    while (m_action != ACTION_CANCEL)
    {
        if (m_action == ACTION_CREATE)
        {
            NewSessionDialog *dialog = new NewSessionDialog(m_parent);
            int response = dialog->run();
            if (response == GTK_RESPONSE_OK)
            {
                m_sessionName = dialog->name();
                delete dialog;
                return;
            }
            delete dialog;
            if (response == RESPONSE_RESTORE_SESSION)
                m_action = ACTION_RESTORE;
            else
                m_action = ACTION_CANCEL;
        }
        else
        {
            RestoreSessionDialog *dialog = new RestoreSessionDialog(m_parent);
            int response = dialog->run();
            if (response == GTK_RESPONSE_OK)
            {
                m_sessionName = dialog->name();
                delete dialog;
                return;
            }
            delete dialog;
            if (response == RESPONSE_NEW_SESSION)
                m_action = ACTION_CREATE;
            else
                m_action = ACTION_CANCEL;
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
