// Session chooser dialog.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ session-chooser-dialog.cpp -DSMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST\
 `pkg-config --cflags --libs gtk+-3.0` -Werror -Wall -o session-chooser-dialog
*/

#ifdef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST
# define _(T) T
#endif

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session-chooser-dialog.hpp"
#ifndef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST
# include "../../session.hpp"
#endif
#include "../../utilities/misc.hpp"
#include <string>
#include <vector>
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

bool readSessions(std::vector<std::string> &sessionNames,
                  std::vector<Samoyed::Session::LockState> &sessionStates)
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
    sessionStates.reserve(sessionNames.size());
    for (std::vector<std::string>::const_iterator it = sessionNames.begin();
         it != sessionNames.end();
         ++it)
        sessionStates.push_back(Samoyed::Session::queryLockState(state));
    return successful;
#endif
}

}

namespace Samoyed
{

void SessionChooserDialog::NewSessionDialog::onResponse(GtkDialog *dialog,
                                                        gint response,
                                                        gpointer d)
{

}

SessionChooserDialog::NewSessionDialog::NewSessionDialog()
{
    GtkWidget *label = gtk_label_new_with_mnemonic(_("_Session name:"));
    m_name = gtk_entry_new();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), m_name);
    GtkGrid *grid = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            label, NULL,
                            GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(grid),
                            m_name, label,
                            GTK_POS_RIGHT, 1, 1);
    gtk_container_set_column_spacing(GTK_CONTAINER(grid), LABEL_SPACING);
    m_dialog = gtk_dialog_new_with_buttons(
        _("New Session"),
        NULL,
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
    gtk_dialog_set_default_response(GTK_DIALOG(m_dialog), GTK_RESPONSE_YES);
    g_signal_connect(m_dialog, "response", G_CALLBACK(onResponse), this);
    gtk_widget_show_all(grid);
}

SessionChooserDialog::NewSessionDialog::~NewSessionDialog()
{
    gtk_widget_destroy(m_dialog);
}

int SessionChooserDialog::NewSessionDialog::run()
{
    return gtk_dialog_run(GTK_DIALOG(m_dialog));
}

const char *SessionChooserDialog::NewSessionDialog::name() const
{
    return gtk_entry_get_text(GTK_ENTRY(m_name));
}

void SessionChooserDialog::onSwitchButtonClicked(GtkButton *button,
                                                 gpointer dialog)
{
    SessionChooserDialog *d = static_cast<SessionChooserDialog *>(dialog);
    if (d->m_action == ACTION_CREATE)
    {
        d->m_action = ACTION_RESTORE;
        gtk_window_set_title(GTK_WINDOW(d->m_dialog), _("Restore Session"));
        gtk_notebook_set_current_page(GTK_NOTEBOOK(d->m_notebook), 1);
        gtk_button_set_label(button, _("To _New Session"));
        gtk_widget_set_tooltip_text(
            GTK_WIDGET(button),
            _("Open a dialog to create a new session"));
    }
    else
    {
        d->m_action = ACTION_CREATE;
        gtk_window_set_title(GTK_WINDOW(d->m_dialog), _("New Session"));
        gtk_notebook_set_current_page(GTK_NOTEBOOK(d->m_notebook), 0);
        gtk_button_set_label(button, _("To _Restore Session"));
        gtk_widget_set_tooltip_text(
            GTK_WIDGET(button),
            _("Open a dialog to choose a saved session to restore"));
    }
}

void
SessionChooserDialog::onSelectedSessionChanged(GtkTreeSelection *selection,
                                               gpointer dialog)
{
    GtkTreeModel *model;
    GtkTreeIter it;
    char *sessionName;
    SessionChooserDialog *d = static_cast<SessionChooserDialog *>(dialog);
    if (gtk_tree_selection_get_selected(selection, &model, &it))
    {
        gtk_tree_model_get(model, &it, 0, &sessionName, -1);
        gtk_entry_set_text(GTK_ENTRY(d->m_newSessionEntry), sessionName);
    }
}

SessionChooserDialog::SessionChooserDialog(Action action):
    m_action(action)
{
    // The dialog.
    m_dialog = gtk_dialog_new_with_buttons(
        action == ACTION_CREATE ? _("New Session") : _("Restore Session"),
        NULL,
        GTK_DIALOG_MODAL,
        GTK_STOCK_OK, GTK_RESPONSE_OK,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        NULL);

    // The notebook.
    m_notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(m_notebook), FALSE);
    gtk_container_add(
        GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(m_dialog))),
        m_notebook);

    // The sessions data.
    std::vector<std::string> sessionNames;
    std::vector<Session::LockState> sessionStates;
    readSessions(sessionNames, sessionStates);
    m_sessions = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_BOOLEAN);
    GtkTreeIter it;
    for (std::vector<std::string>::size_type i = 0;
         i < sessionNames.size();
         ++i)
    {
        gtk_list_store_append(m_sessions, &it);
        gtk_list_store_set(m_sessions, &it,
                           0, sessionNames[i].c_str(),
                           1, sessionStates[i] != Session::STATE_UNLOCKED,
                           -1);
    }

    // The new session page.
    GtkWidget *label = gtk_label_new_with_mnemonic(_("_Session name:"));
    m_newSessionEntry = gtk_entry_new();
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), m_newSessionEntry);
    GtkWidget *labelEntry = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(labelEntry),
                            label, NULL,
                            GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GKT_GRID(labelEntry),
                            m_newSessionEntry, label,
                            GTK_POS_RIGHT, 1, 1);
    label = gtk_label_new_with_mnemonic(_("S_aved sessions:"));
    GtkWidget *newPage = gtk_grid_new();
gtk_grid_
    gtk_grid_attach_next_to(GTK_GRID(newPage),
                            m_newSessionEntry, NULL,
                            GTK_POS_BOTTOM, 1, 1);
    GtkWidget *expander = gtk_expander_new_with_mnemonic(_("_Saved sessions"));
    gtk_expander_set_spacing(GTK_EXPANDER(expander), EXPANDER_SPACING);
    gtk_expander_set_resize_toplevel(GTK_EXPANDER(expander), TRUE);
    gtk_grid_attach_next_to(GTK_GRID(newPage),
                            expander, m_newSessionEntry,
                            GTK_POS_BOTTOM, 1, 1);
    GtkWidget *sessionList =
        gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_sessions));
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Name",
                                                      renderer,
                                                      "text", 0,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sessionList), column);
    renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_activatable(
        GTK_CELL_RENDERER_TOGGLE(renderer), FALSE);
    column = gtk_tree_view_column_new_with_attributes("Locked",
                                                      renderer,
                                                      "active", 1,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sessionList), column);
    gtk_container_add(GTK_CONTAINER(expander), sessionList);
    GtkTreeSelection *selection;
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sessionList));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);
    g_signal_connect(selection, "changed",
                     G_CALLBACK(onSelectedSessionChanged), this);

    // The restore session page.
    sessionList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_sessions));
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes("Name",
                                                      renderer,
                                                      "text", 0,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sessionList), column);
    renderer = gtk_cell_renderer_toggle_new();
    gtk_cell_renderer_toggle_set_activatable(
        GTK_CELL_RENDERER_TOGGLE(renderer), FALSE);
    column = gtk_tree_view_column_new_with_attributes("Locked",
                                                      renderer,
                                                      "active", 1,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sessionList), column);
    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sessionList));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_SINGLE);

    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), newPage, NULL);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), restorePage, NULL);

    // The switch button.
    m_switchButton = gtk_button_new_with_mnemonic(
        action == ACTION_CREATE ?
        _("To _Restore Session") : _("To _New Session"));
    gtk_button_set_use_underline(GTK_BUTTON(m_switchButton), TRUE);
    gtk_widget_set_tooltip_text(
        m_switchButton,
        action == ACTION_CREATE ?
        _("Open a dialog to choose a saved session to restore") :
        _("Open a dialog to create a new session"));
    gtk_box_pack_end(
        GTK_BOX(gtk_dialog_get_action_area(GTK_DIALOG(m_dialog))),
        m_switchButton,
        FALSE,
        TRUE,
        0);
    gtk_button_box_set_child_secondary(
        GTK_BUTTON_BOX(gtk_dialog_get_action_area(GTK_DIALOG(m_dialog))),
        m_switchButton, TRUE);
    g_signal_connect(m_switchButton, "clicked",
                     G_CALLBACK(onSwitchButtonClicked), this);

    gtk_notebook_set_current_page(
        GTK_NOTEBOOK(m_notebook),
        action == ACTION_CREATE ? 0 : 1);
    gtk_widget_show_all(m_notebook);
    gtk_widget_show(m_switchButton);
}

SessionChooserDialog::~SessionChooserDialog()
{
    gtk_widget_destroy(m_dialog);
    g_object_unref(m_sessions);
}

void SessionChooserDialog::run()
{
    gtk_dialog_run(GTK_DIALOG(m_dialog));
}

}

#ifdef SMYD_SESSION_CHOOSER_DIALOG_UNIT_TEST

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    Samoyed::SessionChooserDialog *dialog = new Samoyed::SessionChooserDialog(
        Samoyed::SessionChooserDialog::ACTION_CREATE);
    dialog->run();
    delete dialog;
    dialog = new Samoyed::SessionChooserDialog(
        Samoyed::SessionChooserDialog::ACTION_RESTORE);
    dialog->run();
    delete dialog;
    return 0;
}

#endif
