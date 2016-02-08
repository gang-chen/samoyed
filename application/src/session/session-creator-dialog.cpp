// Session creator dialog.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session-creator-dialog.hpp"
#include "application.hpp"
#include <string>
#include <gtk/gtk.h>

namespace Samoyed
{

void SessionCreatorDialog::validateInput(SessionCreatorDialog *dialog)
{
    gtk_dialog_set_response_sensitive(
        dialog->m_dialog,
        GTK_RESPONSE_ACCEPT,
        *dialog->sessionName());
}

SessionCreatorDialog::SessionCreatorDialog(GtkWindow *parent)
{
    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "session-creator-dialog.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());

    m_dialog =
        GTK_DIALOG(gtk_builder_get_object(m_builder, "session-creator-dialog"));
    if (parent)
    {
        gtk_window_set_transient_for(GTK_WINDOW(m_dialog),
                                     parent);
        gtk_window_set_modal(GTK_WINDOW(m_dialog), TRUE);
    }

    m_sessionNameEntry =
        GTK_ENTRY(gtk_builder_get_object(m_builder,
                                         "session-name-entry"));
    g_signal_connect_swapped(GTK_EDITABLE(m_sessionNameEntry), "changed",
                             G_CALLBACK(validateInput), this);

    validateInput(this);
}

SessionCreatorDialog::~SessionCreatorDialog()
{
    gtk_widget_destroy(GTK_WIDGET(m_dialog));
    g_object_unref(m_builder);
}

bool SessionCreatorDialog::run()
{
    return gtk_dialog_run(m_dialog) == GTK_RESPONSE_ACCEPT;
}

const char *SessionCreatorDialog::sessionName() const
{
    return gtk_entry_get_text(m_sessionNameEntry);
}

}
