// Project creator dialog.
// Copyright (C) 2015 Gnag chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-creator-dialog.hpp"
#include "application.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

ProjectCreatorDialog::ProjectCreatorDialog()
{
}

ProjectCreatorDialog::~ProjectCreatorDialog()
{
}

bool ProjectCreatorDialog::run()
{
    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "project-creator-dialog.xml";
    GtkBuilder *builder = gtk_builder_new_from_file(uiFile.c_str());
    GtkWidget *ass = GTK_WIDGET(gtk_builder_get_object(builder, "assistant"));
    gtk_widget_show_all(ass);
}

}
