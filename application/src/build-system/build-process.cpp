// Build process.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "build-process.hpp"
#include "build-system.hpp"
#include "build-log-view.hpp"
#include "build-log-view-group.hpp"
#include "project/project.hpp"
#include "utilities/miscellaneous.hpp"
#include "window/window.hpp"
#include "application.hpp"
#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>

namespace
{

const char *actionText[] =
{
    N_("configure"),
    N_("build"),
    N_("install")
};

const char *actionText2[] =
{
    N_("configuring"),
    N_("building"),
    N_("installing")
};

}

namespace Samoyed
{

BuildProcess::BuildProcess(BuildSystem &buildSystem,
                           const char *configName,
                           const char *commands,
                           Action action):
    m_buildSystem(buildSystem),
    m_configName(configName),
    m_commands(commands),
    m_action(action),
    m_logView(NULL)
{
}

BuildProcess::~BuildProcess()
{
    if (running())
        stop();

    GError *error;
    GIOStatus status;
    error = NULL;
    status = closeStdout(&error);
    if (status != G_IO_STATUS_NORMAL)
    {
        if (error)
        {
            g_warning(_("Samoyed failed to close the pipe connected to the "
                        "stdout of the child process when %s project \"%s\" "
                        "with configuration \"%s\": %s."),
                      gettext(actionText2[m_action]),
                      m_buildSystem.project().uri(),
                      m_configName.c_str(),
                      error->message);
            g_error_free(error);
        }
        else
            g_warning(_("Samoyed failed to close the pipe connected to the "
                        "stdout of the child process when %s project \"%s\" "
                        "with configuration \"%s\"."),
                      gettext(actionText2[m_action]),
                      m_buildSystem.project().uri(),
                      m_configName.c_str());
    }
    error = NULL;
    status = closeStderr(&error);
    if (status != G_IO_STATUS_NORMAL)
    {
        if (error)
        {
            g_warning(_("Samoyed failed to close the pipe connected to the "
                        "stderr of the child process when %s project "
                        "\"%s\" with configuration \"%s\": %s."),
                      gettext(actionText2[m_action]),
                      m_buildSystem.project().uri(),
                      m_configName.c_str(),
                      error->message);
            g_error_free(error);
        }
        else
            g_warning(_("Samoyed failed to close the pipe connected to the "
                        "stderr of the child process when %s project \"%s\" "
                        "with configuration \"%s\"."),
                      gettext(actionText2[m_action]),
                      m_buildSystem.project().uri(),
                      m_configName.c_str());
    }

    if (m_logView)
        m_logViewClosedConn.disconnect();
}

bool BuildProcess::run()
{
    char *pwd = g_filename_from_uri(m_buildSystem.project().uri(), NULL, NULL);
    const char *argv[4];
    boost::shared_ptr<char> shell = findShell();
    argv[0] = shell.get();
    argv[1] = "-c";
    argv[2] = m_commands.c_str();
    argv[3] = NULL;
    GError *error = NULL;
    if (!Process::run(pwd,
                      argv,
                      NULL,
                      onFinished,
                      boost::function<void (Process &, GIOChannel *, int)>(),
                      onStdout,
                      onStderr,
                      &error))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to start a child process to %s project \"%s\" "
              "with configuration \"%s\"."),
            gettext(actionText[m_action]),
            m_buildSystem.project().uri(),
            m_configName.c_str());
        if (error)
        {
            gtkMessageDialogAddDetails(dialog, error->message);
            g_error_free(error);
        }
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }

    // Show the build log view.
    BuildLogViewGroup *group =
        Application::instance().currentWindow()->openBuildLogViewGroup();
    m_logView = group->openBuildLogView(m_buildSystem.project().uri(),
                                        m_configName.c_str());
    m_logView->startBuild(m_action);
    m_logView->clear();
    m_logViewClosedConn = m_logView->addClosedCallback(
        boost::bind(onLogViewClosed, this, _1));

    return true;
}

void BuildProcess::stop()
{
    Process::stop();
    if (m_logView)
        m_logView->onBuildStopped();
}

void BuildProcess::onFinished(Process &process, int exitCode)
{
    BuildProcess &proc = static_cast<BuildProcess &>(process);
    if (proc.m_logView)
        proc.m_logView->onBuildFinished(exitCode);
    proc.m_buildSystem.onBuildFinished(proc.m_configName.c_str());
}

void BuildProcess::onStdout(Process &process,
                            GIOChannel *channel,
                            int condition)
{
    BuildProcess &proc = static_cast<BuildProcess &>(process);
    if (((condition & G_IO_IN) || (condition & G_IO_PRI)) && proc.m_logView)
    {
        gchar *log = NULL;
        gsize length;
        GError *error = NULL;
        GIOStatus status;
        if (condition & G_IO_HUP)
            status = g_io_channel_read_to_end(channel,
                                              &log,
                                              &length,
                                              &error);
        else
            status = g_io_channel_read_line(channel,
                                            &log,
                                            &length,
                                            NULL,
                                            &error);
        if (status == G_IO_STATUS_NORMAL && log)
            proc.m_logView->addLog(log, length);
        g_free(log);
        if (status == G_IO_STATUS_ERROR)
        {
            if (error)
            {
                g_warning(_("Samoyed failed to read the stdout of the child "
                            "process when %s project \"%s\" with configuration "
                            "\"%s\": %s."),
                            gettext(actionText2[proc.m_action]),
                            proc.m_buildSystem.project().uri(),
                            proc.m_configName.c_str(),
                            error->message);
                g_error_free(error);
            }
            else
                g_warning(_("Samoyed failed to read the stdout of the child "
                            "process when %s project \"%s\" with configuration "
                            "\"%s\"."),
                            gettext(actionText2[proc.m_action]),
                            proc.m_buildSystem.project().uri(),
                            proc.m_configName.c_str());
        }
    }
    if (condition & G_IO_ERR)
        g_warning(_("Samoyed got a G_IO_ERR error on the stdout of the child "
                    "process when %s project \"%s\" with configuration "
                    "\"%s\"."),
                  gettext(actionText2[proc.m_action]),
                  proc.m_buildSystem.project().uri(),
                  proc.m_configName.c_str());
    if (condition & G_IO_NVAL)
        g_warning(_("Samoyed got a G_IO_NVAL error on the stdout of the child "
                    "process when %s project \"%s\" with configuration "
                    "\"%s\"."),
                  gettext(actionText2[proc.m_action]),
                  proc.m_buildSystem.project().uri(),
                  proc.m_configName.c_str());
}

void BuildProcess::onStderr(Process &process,
                            GIOChannel *channel,
                            int condition)
{
    BuildProcess &proc = static_cast<BuildProcess &>(process);
    if (((condition & G_IO_IN) || (condition & G_IO_PRI)) && proc.m_logView)
    {
        gchar *log = NULL;
        gsize length;
        GError *error = NULL;
        GIOStatus status;
        if (condition & G_IO_HUP)
            status = g_io_channel_read_to_end(channel,
                                              &log,
                                              &length,
                                              &error);
        else
            status = g_io_channel_read_line(channel,
                                            &log,
                                            &length,
                                            NULL,
                                            &error);
        if (status == G_IO_STATUS_NORMAL && log)
            proc.m_logView->addLog(log, length);
        g_free(log);
        if (status == G_IO_STATUS_ERROR)
        {
            if (error)
            {
                g_warning(_("Samoyed failed to read the stderr of the child "
                            "process when %s project \"%s\" with configuration "
                            "\"%s\": %s."),
                            gettext(actionText2[proc.m_action]),
                            proc.m_buildSystem.project().uri(),
                            proc.m_configName.c_str(),
                            error->message);
                g_error_free(error);
            }
            else
                g_warning(_("Samoyed failed to read the stderr of the child "
                            "process when %s project \"%s\" with configuration "
                            "\"%s\"."),
                            gettext(actionText2[proc.m_action]),
                            proc.m_buildSystem.project().uri(),
                            proc.m_configName.c_str());
        }
    }
    if (condition & G_IO_ERR)
        g_warning(_("Samoyed got a G_IO_ERR error on the stderr of the child "
                    "process when %s project \"%s\" with configuration "
                    "\"%s\"."),
                  gettext(actionText2[proc.m_action]),
                  proc.m_buildSystem.project().uri(),
                  proc.m_configName.c_str());
    if (condition & G_IO_NVAL)
        g_warning(_("Samoyed got a G_IO_NVAL error on the stderr of the child "
                    "process when %s project \"%s\" with configuration "
                    "\"%s\"."),
                  gettext(actionText2[proc.m_action]),
                  proc.m_buildSystem.project().uri(),
                  proc.m_configName.c_str());
}

void BuildProcess::onLogViewClosed(Widget &widget)
{
    m_logView = NULL;
}

}
