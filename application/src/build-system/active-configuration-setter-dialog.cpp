// Active configuration setter dialog.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "active-configuration-setter-dialog.hpp"
#include "build-system.hpp"
#include "configuration.hpp"
#include "application.hpp"
#include "project/project.hpp"
#include <map>
#include <gtk/gtk.h>

namespace Samoyed
{

void ActiveConfigurationSetterDialog::validateInput(
    gpointer object,
    ActiveConfigurationSetterDialog *dialog)
{
    if (!gtk_combo_box_get_active_id(
            GTK_COMBO_BOX(dialog->m_configChooser)))
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

ActiveConfigurationSetterDialog::ActiveConfigurationSetterDialog(
    GtkWindow *parent,
    BuildSystem &buildSystem):
        m_buildSystem(buildSystem)
{
    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "active-configuration-setter-dialog.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());
    m_dialog = GTK_DIALOG(gtk_builder_get_object(
        m_builder,
        "active-configuration-setter-dialog"));
    m_configChooser =
        GTK_COMBO_BOX_TEXT(gtk_builder_get_object(m_builder,
                                                  "configuration-chooser"));
    gtk_label_set_label(
        GTK_LABEL(gtk_builder_get_object(m_builder, "project-uri-label")),
        m_buildSystem.project().uri());

    const BuildSystem::ConfigurationTable &configs =
        m_buildSystem.configurations();
    int i = 0;
    for (BuildSystem::ConfigurationTable::const_iterator it = configs.begin();
         it != configs.end();
         ++it)
        gtk_combo_box_text_insert(m_configChooser,
                                  i++,
                                  it->first,
                                  it->first);
    Configuration *config = m_buildSystem.activeConfiguration();
    if (config)
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(m_configChooser),
                                    config->name());

    if (parent)
    {
        gtk_window_set_transient_for(GTK_WINDOW(m_dialog), parent);
        gtk_window_set_modal(GTK_WINDOW(m_dialog), TRUE);
    }

    g_signal_connect(GTK_COMBO_BOX(m_configChooser), "changed",
                     G_CALLBACK(validateInput), this);

    validateInput(NULL, this);
}

ActiveConfigurationSetterDialog::~ActiveConfigurationSetterDialog()
{
    gtk_widget_destroy(GTK_WIDGET(m_dialog));
    g_object_unref(m_builder);
}

void ActiveConfigurationSetterDialog::run()
{
    if (gtk_dialog_run(m_dialog) != GTK_RESPONSE_ACCEPT)
        return;

    const char *configName = gtk_combo_box_get_active_id(
        GTK_COMBO_BOX(m_configChooser));
    Configuration *config = m_buildSystem.configurations()[configName];
    m_buildSystem.setActiveConfiguration(config);
}

}
