// Build log view.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "build-log-view.hpp"
#include "build-system.hpp"
#include "project/project.hpp"
#include "application.hpp"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#define BUILD_LOG_VIEW_ID "build-log-view"

namespace
{

const char *actionText[] =
{
    N_("Configuring"),
    N_("Building"),
    N_("Installing")
};

const char *actionText2[] =
{
    N_("configuring"),
    N_("building"),
    N_("installing")
};

const char *actionText3[] =
{
    N_("configure"),
    N_("build"),
    N_("install")
};

}

namespace Samoyed
{

bool BuildLogView::setup()
{
    if (!Widget::setup(BUILD_LOG_VIEW_ID))
        return false;
    char *projFileName = g_filename_from_uri(m_projectUri.c_str(), NULL, NULL);
    char *projBaseName = g_path_get_basename(projFileName);
    char *title = g_strdup_printf(_("%s (%s)"),
                                  projBaseName,
                                  m_configName.c_str());
    setTitle(title);
    g_free(projFileName);
    g_free(projBaseName);
    g_free(title);
    char *desc = g_strdup_printf(_("%s (%s)"),
                                 m_projectUri.c_str(),
                                 m_configName.c_str());
    setDescription(desc);
    g_free(desc);
    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S "build-log-view.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());
    m_message = GTK_LABEL(gtk_builder_get_object(m_builder, "message"));
    m_stopButton = GTK_BUTTON(gtk_builder_get_object(m_builder, "stop-button"));
    g_signal_connect(m_stopButton, "clicked",
                     G_CALLBACK(onStopBuild), this);
    m_log = GTK_TEXT_VIEW(gtk_builder_get_object(m_builder, "log"));
    GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(m_builder, "grid"));
    setGtkWidget(grid);
    gtk_widget_show_all(grid);
    return true;
}

BuildLogView *BuildLogView::create(const char *projectUri,
                                   const char *configName)
{
    BuildLogView *view = new BuildLogView(projectUri, configName);
    if (!view->setup())
    {
        delete view;
        return NULL;
    }
    return view;
}

void BuildLogView::startBuild(Builder::Action action)
{
    m_action = action;
    char *message = g_strdup_printf(
        _("%s project \"%s\" with configuration \"%s\"."),
        gettext(actionText[m_action]),
        m_projectUri.c_str(),
        m_configName.c_str());
    gtk_label_set_label(m_message, message);
    g_free(message);
    gtk_widget_show(GTK_WIDGET(m_stopButton));
}

void BuildLogView::clear()
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(m_log);
    GtkTextIter begin, end;
    gtk_text_buffer_get_start_iter(buffer, &begin);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_delete(buffer, &begin, &end);
}

void BuildLogView::addLog(const char *log, int length)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(m_log);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, log, length);
}

void BuildLogView::onBuildFinished(bool successful, const char *error)
{
    char *message;
    if (successful)
        message = g_strdup_printf(
            _("Successfully finished %s project \"%s\" with configuration "
              "\"%s\"."),
            gettext(actionText2[m_action]),
            m_projectUri.c_str(),
            m_configName.c_str());
    else
        message = g_strdup_printf(
            _("Failed to %s project \"%s\" with configuration \"%s\". %s."),
            gettext(actionText3[m_action]),
            m_projectUri.c_str(),
            m_configName.c_str(),
            error);
    gtk_label_set_label(m_message, message);
    g_free(message);
    gtk_widget_hide(GTK_WIDGET(m_stopButton));
}

void BuildLogView::onBuildStopped()
{
    char *message = g_strdup_printf(
        _("Stopped %s project \"%s\" with configuration \"%s\"."),
        gettext(actionText2[m_action]),
        m_projectUri.c_str(),
        m_configName.c_str());
    gtk_label_set_label(m_message, message);
    g_free(message);
    gtk_widget_hide(GTK_WIDGET(m_stopButton));
}

void BuildLogView::onStopBuild(GtkButton *button, gpointer view)
{
    BuildLogView *v = static_cast<BuildLogView *>(view);
    Project *project =
        Application::instance().findProject(v->m_projectUri.c_str());
    if (!project)
    {
        v->onBuildStopped();
        return;
    }
    project->buildSystem().stopBuild(v->m_configName.c_str());
}

}
