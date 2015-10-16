// Configuration creator dialog.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "configuration-creator-dialog.hpp"
#include "configuration.hpp"
#include "build-system.hpp"
#include "project/project.hpp"
#include "application.hpp"
#include <string>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

namespace Samoyed
{

void
ConfigurationCreatorDialog::validateInput(gpointer object,
                                          ConfigurationCreatorDialog *dialog)
{
    if (*gtk_entry_get_text(dialog->m_nameEntry) == '\0')
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

ConfigurationCreatorDialog::ConfigurationCreatorDialog(
    GtkWindow *parent,
    BuildSystem &buildSystem):
        m_buildSystem(buildSystem)
{
    Configuration config(m_buildSystem.defaultConfiguration());

    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "configuration-creator-dialog.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());
    m_dialog =
        GTK_DIALOG(gtk_builder_get_object(m_builder,
                                          "configuration-creator-dialog"));
    m_nameEntry =
        GTK_ENTRY(gtk_builder_get_object(m_builder, "name-entry"));
    m_configCommandsEntry =
        GTK_ENTRY(gtk_builder_get_object(m_builder,
                                         "configure-commands-entry"));
    gtk_entry_set_text(m_configCommandsEntry, config.configureCommands());
    m_buildCommandsEntry =
        GTK_ENTRY(gtk_builder_get_object(m_builder, "build-commands-entry"));
    gtk_entry_set_text(m_buildCommandsEntry, config.buildCommands());
    m_installCommandsEntry =
        GTK_ENTRY(gtk_builder_get_object(m_builder, "install-commands-entry"));
    gtk_entry_set_text(m_installCommandsEntry, config.installCommands());
    m_cCompilerEntry =
        GTK_ENTRY(gtk_builder_get_object(m_builder, "c-compiler-entry"));
    gtk_entry_set_text(m_cCompilerEntry, config.cCompiler());
    m_cppCompilerEntry =
        GTK_ENTRY(gtk_builder_get_object(m_builder, "c++-compiler-entry"));
    gtk_entry_set_text(m_cppCompilerEntry, config.cppCompiler());
    m_cCompilerOptionsEntry =
        GTK_ENTRY(gtk_builder_get_object(m_builder,
                                         "c-compiler-options-entry"));
    gtk_entry_set_text(m_cCompilerOptionsEntry, config.cCompilerOptions());
    m_cppCompilerOptionsEntry =
        GTK_ENTRY(gtk_builder_get_object(m_builder,
                                         "c++-compiler-options-entry"));
    gtk_entry_set_text(m_cppCompilerOptionsEntry, config.cppCompilerOptions());
    gtk_label_set_label(
        GTK_LABEL(gtk_builder_get_object(m_builder, "project-uri-label")),
        m_buildSystem.project().uri());

    if (parent)
    {
        gtk_window_set_transient_for(GTK_WINDOW(m_dialog), parent);
        gtk_window_set_modal(GTK_WINDOW(m_dialog), TRUE);
    }

    g_signal_connect(GTK_EDITABLE(m_nameEntry), "changed",
                     G_CALLBACK(validateInput), this);

    validateInput(NULL, this);
}

ConfigurationCreatorDialog::~ConfigurationCreatorDialog()
{
    gtk_widget_destroy(GTK_WIDGET(m_dialog));
    g_object_unref(m_builder);
}

Configuration *ConfigurationCreatorDialog::run()
{
    if (gtk_dialog_run(m_dialog) != GTK_RESPONSE_ACCEPT)
        return NULL;

    Configuration *config = new Configuration;
    config->setName(gtk_entry_get_text(m_nameEntry));
    config->setConfigureCommands(gtk_entry_get_text(m_configCommandsEntry));
    config->setBuildCommands(gtk_entry_get_text(m_buildCommandsEntry));
    config->setInstallCommands(gtk_entry_get_text(m_installCommandsEntry));
    config->setCCompiler(gtk_entry_get_text(m_cCompilerEntry));
    config->setCppCompiler(gtk_entry_get_text(m_cppCompilerEntry));
    config->setCCompilerOptions(gtk_entry_get_text(m_cCompilerOptionsEntry));
    config->setCppCompilerOptions(
        gtk_entry_get_text(m_cppCompilerOptionsEntry));
    if (!m_buildSystem.addConfiguration(*config))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(m_dialog),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create configuration \"%s\" for project "
              "\"%s\" because configuration \"%s\" already exists."),
            config->name(),
            m_buildSystem.project().uri(),
            config->name());
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        delete config;
        return NULL;
    }

    return config;
}

}
