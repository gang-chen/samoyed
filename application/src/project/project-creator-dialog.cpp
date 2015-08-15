// Project creator dialog.
// Copyright (C) 2015 Gnag chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-creator-dialog.hpp"
#include "project.hpp"
#include "build-system/build-systems-extension-point.hpp"
#include "plugin/extension-point-manager.hpp"
#include "application.hpp"
#include <string>
#include <gtk/gtk.h>

#define BUILD_SYSTEMS "build-systems"

namespace Samoyed
{

void ProjectCreatorDialog::validateInput(gpointer object,
                                         ProjectCreatorDialog *dialog)
{
    char *dir = gtk_file_chooser_get_uri(dialog->m_locationChooser);
    if (!dir)
    {
        gtk_widget_set_sensitive(
            gtk_dialog_get_widget_for_response(dialog->m_dialog,
                                               GTK_RESPONSE_ACCEPT),
            FALSE);
        return;
    }
    g_free(dir);
    if (!gtk_combo_box_get_active_id(
            GTK_COMBO_BOX(dialog->m_buildSystemChooser)))
    {
        gtk_widget_set_sensitive(
            gtk_dialog_get_widget_for_response(dialog->m_dialog,
                                               GTK_RESPONSE_ACCEPT),
            FALSE);
        return;
    }
    gtk_widget_set_sensitive(
        gtk_dialog_get_widget_for_response(dialog->m_dialog,
                                           GTK_RESPONSE_ACCEPT),
        TRUE);
}

ProjectCreatorDialog::ProjectCreatorDialog(GtkWindow *parent)
{
    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "project-creator-dialog.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());
    m_dialog =
        GTK_DIALOG(gtk_builder_get_object(m_builder, "project-creator-dialog"));
    m_locationChooser =
        GTK_FILE_CHOOSER(gtk_builder_get_object(m_builder,
                                                "location-chooser"));
    m_buildSystemChooser =
        GTK_COMBO_BOX_TEXT(gtk_builder_get_object(m_builder,
                                                  "build-system-chooser"));
    if (parent)
    {
        gtk_window_set_transient_for(GTK_WINDOW(m_dialog), parent);
        gtk_window_set_modal(GTK_WINDOW(m_dialog), true);
    }

    // Add the registered build systems.
    const BuildSystemsExtensionPoint::ExtensionTable &buildSystems =
        static_cast<BuildSystemsExtensionPoint &>(Application::instance().
        extensionPointManager().extensionPoint(BUILD_SYSTEMS)).extensions();
    int i = 0;
    for (BuildSystemsExtensionPoint::ExtensionTable::const_iterator it =
            buildSystems.begin();
         it != buildSystems.end();
         ++it)
        gtk_combo_box_text_insert(m_buildSystemChooser,
                                  i++,
                                  it->second->id.c_str(),
                                  it->second->description.c_str());

    g_signal_connect(m_locationChooser, "selection-changed",
                     G_CALLBACK(validateInput), this);
    g_signal_connect(GTK_COMBO_BOX(m_buildSystemChooser), "changed",
                     G_CALLBACK(validateInput), this);
}

ProjectCreatorDialog::~ProjectCreatorDialog()
{
    gtk_widget_destroy(GTK_WIDGET(m_dialog));
    g_object_unref(m_builder);
}

Project *ProjectCreatorDialog::run()
{
    if (gtk_dialog_run(m_dialog) != GTK_RESPONSE_ACCEPT)
        return NULL;
    Project *project;
    char *uri = gtk_file_chooser_get_uri(m_locationChooser);
    const char *buildSystem =
        gtk_combo_box_get_active_id(GTK_COMBO_BOX(m_buildSystemChooser));
    project = Project::create(uri, buildSystem);
    g_free(uri);
    return project;
}

}
