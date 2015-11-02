// Configuration management window.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "configuration-management-window.hpp"
#include "configuration.hpp"
#include "build-system.hpp"
#include "project/project.hpp"
#include "application.hpp"
#include <string>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

namespace
{

gboolean onDeleteEvent(GtkWidget *widget,
                       GdkEvent *event,
                       Samoyed::ConfigurationManagementWindow *window)
{
    delete window;
    return TRUE;
}

}

namespace Samoyed
{

void ConfigurationManagementWindow::onEditConfiguration(
    GtkButton *button,
    ConfigurationManagementWindow *window)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter it;

    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(window->m_projectList));
    if (!gtk_tree_selection_get_selected(selection, &model, &it))
    {
        GtkWidget *d = gtk_message_dialog_new(
            window->m_window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("No project is chosen. Choose the project whose configuration "
              "you want to edit from the project list."));
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    char *projectUri;
    gtk_tree_model_get(model, &it, 0, &projectUri, -1);

    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(window->m_configurationList));
    if (!gtk_tree_selection_get_selected(selection, &model, &it))
    {
        GtkWidget *d = gtk_message_dialog_new(
            window->m_window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("No configuration is chosen. Choose the configuration you want "
              "to edit from the configuration list."));
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(projectUri);
        return;
    }
    char *configName;
    gtk_tree_model_get(model, &it, 0, &configName, -1);

    Project *project = Application::instance().findProject(projectUri);
    if (!project)
    {
        GtkWidget *d = gtk_message_dialog_new(
            window->m_window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to edit configuration \"%s\" of project \"%s\" "
              "because the project is closed."),
            configName,
            projectUri);
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(projectUri);
        g_free(configName);
        return;
    }

    Configuration *config =
        project->buildSystem().findConfiguration(configName);
    if (!config)
    {
        GtkWidget *d = gtk_message_dialog_new(
            window->m_window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to edit configuration \"%s\" of project \"%s\" "
              "because the project does not have a configuration named "
              "\"%s\"."),
            configName,
            projectUri,
            configName);
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(projectUri);
        g_free(configName);
        return;
    }

    gtk_label_set_label(
        GTK_LABEL(gtk_builder_get_object(window->m_builder,
                                         "project-uri-label")),
        projectUri);
    gtk_label_set_label(
        GTK_LABEL(gtk_builder_get_object(window->m_builder,
                                         "name-content-label")),
        configName);
    gtk_entry_set_text(
        GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                         "configure-commands-entry")),
        config->configureCommands());
    gtk_entry_set_text(
        GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                         "build-commands-entry")),
        config->buildCommands());
    gtk_entry_set_text(
        GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                         "install-commands-entry")),
        config->installCommands());
    gtk_entry_set_text(
        GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                         "clean-commands-entry")),
        config->cleanCommands());
    gtk_entry_set_text(
        GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                         "c-compiler-entry")),
        config->cCompiler());
    gtk_entry_set_text(
        GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                         "c++-compiler-entry")),
        config->cppCompiler());
    gtk_entry_set_text(
        GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                         "c-compiler-options-entry")),
        config->cCompilerOptions());
    gtk_entry_set_text(
        GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                         "c++-compiler-options-entry")),
        config->cppCompilerOptions());

    if (!window->m_editorDialog)
    {
        window->m_editorDialog =
            GTK_DIALOG(gtk_builder_get_object(window->m_builder,
                                              "configuration-editor-dialog"));
        g_signal_connect(GTK_WIDGET(window->m_editorDialog), "delete-event",
                         G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    }

    if (gtk_dialog_run(window->m_editorDialog) == GTK_RESPONSE_ACCEPT)
    {
        config->setConfigureCommands(gtk_entry_get_text(
            GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                             "configure-commands-entry"))));
        config->setBuildCommands(gtk_entry_get_text(
            GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                             "build-commands-entry"))));
        config->setInstallCommands(gtk_entry_get_text(
            GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                             "install-commands-entry"))));
        config->setCleanCommands(gtk_entry_get_text(
            GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                             "clean-commands-entry"))));
        config->setCCompiler(gtk_entry_get_text(
            GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                             "c-compiler-entry"))));
        config->setCppCompiler(gtk_entry_get_text(
            GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                             "c++-compiler-entry"))));
        config->setCCompilerOptions(gtk_entry_get_text(
            GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                             "c-compiler-options-entry"))));
        config->setCppCompilerOptions(gtk_entry_get_text(
            GTK_ENTRY(gtk_builder_get_object(window->m_builder,
                                             "c++-compiler-options-entry"))));
    }
    gtk_widget_hide(GTK_WIDGET(window->m_editorDialog));

    g_free(projectUri);
    g_free(configName);
}

void ConfigurationManagementWindow::deleteSelectedConfiguration(bool confirm)
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter it;

    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(m_projectList));
    if (!gtk_tree_selection_get_selected(selection, &model, &it))
    {
        GtkWidget *d = gtk_message_dialog_new(
            m_window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("No project is chosen. Choose the project whose configuration "
              "you want to delete from the project list."));
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        return;
    }
    char *projectUri;
    gtk_tree_model_get(model, &it, 0, &projectUri, -1);

    selection =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(m_configurationList));
    if (!gtk_tree_selection_get_selected(selection, &model, &it))
    {
        GtkWidget *d = gtk_message_dialog_new(
            m_window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("No configuration is chosen. Choose the configuration you want "
              "to delete from the configuration list."));
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(projectUri);
        return;
    }
    char *configName;
    gtk_tree_model_get(model, &it, 0, &configName, -1);

    if (confirm)
    {
        GtkWidget *d = gtk_message_dialog_new(
            m_window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_QUESTION,
            GTK_BUTTONS_YES_NO,
            _("Are you sure to delete configuration \"%s\" of project \"%s\"?"),
            configName,
            projectUri);
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_NO);
        if (gtk_dialog_run(GTK_DIALOG(d)) != GTK_RESPONSE_YES)
        {
            gtk_widget_destroy(d);
            g_free(configName);
            return;
        }
        gtk_widget_destroy(d);
    }

    Project *project = Application::instance().findProject(projectUri);
    if (!project)
    {
        GtkWidget *d = gtk_message_dialog_new(
            m_window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to delete configuration \"%s\" of project \"%s\" "
              "because the project is closed."),
            configName,
            projectUri);
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(projectUri);
        g_free(configName);
        return;
    }

    Configuration *config =
        project->buildSystem().findConfiguration(configName);
    if (!config)
    {
        GtkWidget *d = gtk_message_dialog_new(
            m_window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to delete configuration \"%s\" of project \"%s\" "
              "because the project does not have a configuration named "
              "\"%s\"."),
            configName,
            projectUri,
            configName);
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(projectUri);
        g_free(configName);
        return;
    }

    if (!project->buildSystem().removeConfiguration(*config))
    {
        GtkWidget *d = gtk_message_dialog_new(
            m_window,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to delete configuration \"%s\" of project \"%s\" "
              "because the configuration is in use."),
            configName,
            projectUri);
        gtk_dialog_set_default_response(GTK_DIALOG(d), GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(d));
        gtk_widget_destroy(d);
        g_free(projectUri);
        g_free(configName);
        return;
    }

    delete config;
    gtk_list_store_remove(
        GTK_LIST_STORE(gtk_builder_get_object(m_builder, "configurations")),
        &it);

    g_free(projectUri);
    g_free(configName);
}

void ConfigurationManagementWindow::onDeleteConfiguration(
    GtkButton *button,
    ConfigurationManagementWindow *window)
{
    window->deleteSelectedConfiguration(true);
}

gboolean ConfigurationManagementWindow::onKeyPress(
    GtkWidget *widget,
    GdkEventKey *event,
    ConfigurationManagementWindow *window)
{
    guint modifiers = gtk_accelerator_get_default_mod_mask();
    if (event->keyval == GDK_KEY_Delete || event->keyval == GDK_KEY_KP_Delete)
    {
        if ((event->state & modifiers) == GDK_SHIFT_MASK)
            window->deleteSelectedConfiguration(false);
        else
            window->deleteSelectedConfiguration(true);
        return TRUE;
    }
    return FALSE;
}

void ConfigurationManagementWindow::onProjectSelectionChanged(
    GtkTreeSelection *selection,
    ConfigurationManagementWindow *window)
{
    GtkListStore *configStore =
        GTK_LIST_STORE(gtk_builder_get_object(window->m_builder,
                                              "configurations"));
    gtk_list_store_clear(configStore);

    GtkTreeModel *model;
    GtkTreeIter it;
    if (!gtk_tree_selection_get_selected(selection, &model, &it))
        return;

    char *projectUri;
    gtk_tree_model_get(model, &it, 0, &projectUri, -1);
    Project *project = Application::instance().findProject(projectUri);
    g_free(projectUri);
    if (!project)
        return;

    for (Configuration *config = project->buildSystem().configurations();
         config;
         config = config->next())
    {
        gtk_list_store_append(configStore, &it);
        gtk_list_store_set(configStore, &it, 0, config->name(), -1);
    }
}

void ConfigurationManagementWindow::onConfigurationSelectionChanged(
    GtkTreeSelection *selection,
    ConfigurationManagementWindow *window)
{
    GtkTreeModel *model;
    GtkTreeIter it;
    if (!gtk_tree_selection_get_selected(selection, &model, &it))
    {
        gtk_widget_set_sensitive(
            GTK_WIDGET(gtk_builder_get_object(window->m_builder,
                                              "edit-config-button")),
            FALSE);
        gtk_widget_set_sensitive(
            GTK_WIDGET(gtk_builder_get_object(window->m_builder,
                                              "delete-config-button")),
            FALSE);
        return;
    }
    gtk_widget_set_sensitive(
        GTK_WIDGET(gtk_builder_get_object(window->m_builder,
                                          "edit-config-button")),
        TRUE);
    gtk_widget_set_sensitive(
        GTK_WIDGET(gtk_builder_get_object(window->m_builder,
                                          "delete-config-button")),
        TRUE);
}

ConfigurationManagementWindow::ConfigurationManagementWindow(GtkWindow *parent):
    m_editorDialog(NULL)
{
    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "configuration-management-window.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());

    GtkListStore *store =
        GTK_LIST_STORE(gtk_builder_get_object(m_builder, "projects"));
    GtkTreeIter it;
    for (const Project *project = Application::instance().projects();
         project;
         project = project->next())
    {
        gtk_list_store_append(store, &it);
        gtk_list_store_set(store, &it, 0, project->uri(), -1);
    }

    m_projectList =
        GTK_TREE_VIEW(gtk_builder_get_object(m_builder, "project-list"));
    g_signal_connect(gtk_tree_view_get_selection(m_projectList),
                     "changed",
                     G_CALLBACK(onProjectSelectionChanged), this);

    m_configurationList =
        GTK_TREE_VIEW(gtk_builder_get_object(m_builder, "configuration-list"));
    g_signal_connect(m_configurationList, "key-press-event",
                     G_CALLBACK(onKeyPress), this);
    g_signal_connect(gtk_tree_view_get_selection(m_configurationList),
                     "changed",
                     G_CALLBACK(onConfigurationSelectionChanged), this);

    g_signal_connect(gtk_builder_get_object(m_builder, "edit-config-button"),
                     "clicked",
                     G_CALLBACK(onEditConfiguration), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "delete-config-button"),
                     "clicked",
                     G_CALLBACK(onDeleteConfiguration), this);

    m_window =
        GTK_WINDOW(gtk_builder_get_object(m_builder,
                                          "configuration-management-window"));
    if (parent)
    {
        gtk_window_set_transient_for(m_window, parent);
        gtk_window_set_destroy_with_parent(m_window, TRUE);
    }
    g_signal_connect(m_window, "delete-event",
                     G_CALLBACK(onDeleteEvent), this);
}

ConfigurationManagementWindow::~ConfigurationManagementWindow()
{
    gtk_widget_destroy(GTK_WIDGET(m_window));
    gtk_widget_destroy(
        GTK_WIDGET(gtk_builder_get_object(m_builder,
                                          "configuration-editor-dialog")));
    g_object_unref(m_builder);
}

void ConfigurationManagementWindow::show()
{
    gtk_widget_show(GTK_WIDGET(m_window));
}

void ConfigurationManagementWindow::hide()
{
    gtk_widget_hide(GTK_WIDGET(m_window));
}

}
