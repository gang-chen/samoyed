// Project creator dialog.
// Copyright (C) 2015 Gnag chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-creator-dialog.hpp"
#include "application.hpp"
#include <string>
#include <gtk/gtk.h>

namespace Samoyed
{

ProjectCreatorDialog::ProjectCreatorDialog(GtkWindow *parent)
{
    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "project-creator-dialog.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());
    m_dialog =
        GTK_DIALOG(gtk_builder_get_object(m_builder, "project-creator-dialog"));
    if (parent)
    {
        gtk_window_set_transient_for(GTK_WINDOW(m_dialog), parent);
        gtk_window_set_modal(GTK_WINDOW(m_dialog), true);
    }
}

ProjectCreatorDialog::~ProjectCreatorDialog()
{
    gtk_widget_destroy(GTK_WIDGET(m_dialog));
    g_object_unref(m_builder);
}

void ProjectCreatorDialog::run()
{
    gtk_dialog_run(m_dialog);
}

}
