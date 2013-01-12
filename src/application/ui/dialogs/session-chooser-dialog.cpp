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
#include <gtk/gtk.h>

namespace Samoyed
{

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

SessionChooserDialog::SessionChooserDialog(Action action):
    m_action(action)
{
    m_dialog = gtk_dialog_new_with_buttons(
        action == ACTION_CREATE ? _("New Session") : _("Restore Session"),
        NULL,
        GTK_DIALOG_MODAL,
        action == ACTION_CREATE ? _("_New") : _("_Restore"), GTK_RESPONSE_OK,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        NULL);

    m_notebook = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(m_notebook), FALSE);
    gtk_container_add(
        GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(m_dialog))),
        m_notebook);

    GtkWidget *page = gtk_label_new("new");
    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), page, NULL);
    page = gtk_label_new("restore");
    gtk_notebook_append_page(GTK_NOTEBOOK(m_notebook), page, NULL);
    gtk_notebook_set_current_page(
        GTK_NOTEBOOK(m_notebook),
        action == ACTION_CREATE ? 0 : 1);

    m_switchButton = gtk_button_new_with_mnemonic(
        action == ACTION_CREATE ?
        _("To _Restore Session") : _("To _New Session"));
    gtk_widget_set_can_default(m_switchButton, TRUE);
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

    gtk_widget_show_all(m_notebook);
    gtk_widget_show(m_switchButton);
}

SessionChooserDialog::~SessionChooserDialog()
{
    gtk_widget_destroy(m_dialog);
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
