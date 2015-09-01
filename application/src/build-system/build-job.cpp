// Build job.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "build-job.hpp"
#include "build-system.hpp"
#include "build-log-view.hpp"
#include "build-log-view-group.hpp"
#include "utilities/miscellaneous.hpp"
#include "window/window.hpp"
#include "application.hpp"
#include <string.h>
#ifdef OS_WINDOWS
# include <windows.h>
#else
# include <sys/types.h>
# include <signal.h>
#endif
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

BuildJob::BuildJob(BuildSystem &buildSystem,
                   const char *commands,
                   Action action,
                   const char *projectUri,
                   const char *configName):
    m_buildSystem(buildSystem),
    m_commands(commands),
    m_action(action),
    m_projectUri(projectUri),
    m_configName(configName),
    m_running(false),
    m_watchingChildStdout(false),
    m_watchingChildStderr(false),
    m_childStdoutChannel(NULL),
    m_childStderrChannel(NULL),
    m_logView(NULL)
{
}

BuildJob::~BuildJob()
{
    if (m_running)
        stop();
    if (m_watchingChildStdout)
        g_source_remove(m_childStdoutWatchId);
    if (m_watchingChildStderr)
        g_source_remove(m_childStderrWatchId);

    GError *error = NULL;
    GIOStatus status;
    if (m_childStdoutChannel)
    {
        status = g_io_channel_shutdown(m_childStdoutChannel, TRUE, &error);
        if (status != G_IO_STATUS_NORMAL)
        {
            if (error)
            {
                g_warning(_("Samoyed failed to close the pipe connected to the "
                            "stdout of the child process when %s project "
                            "\"%s\" (%s): %s."),
                          gettext(actionText2[m_action]),
                          m_projectUri.c_str(),
                          m_configName.c_str(),
                          error->message);
                g_error_free(error);
            }
            else
                g_warning(_("Samoyed failed to close the pipe connected to the "
                            "stdout of the child process when %s project "
                            "\"%s\" (%s)."),
                          gettext(actionText2[m_action]),
                          m_projectUri.c_str(),
                          m_configName.c_str());
        }
        g_io_channel_unref(m_childStdoutChannel);
    }
    if (m_childStderrChannel)
    {
        status = g_io_channel_shutdown(m_childStderrChannel, TRUE, &error);
        if (status != G_IO_STATUS_NORMAL)
        {
            if (error)
            {
                g_warning(_("Samoyed failed to close the pipe connected to the "
                            "stderr of the child process when %s project "
                            "\"%s\" (%s): %s."),
                          gettext(actionText2[m_action]),
                          m_projectUri.c_str(),
                          m_configName.c_str(),
                          error->message);
                g_error_free(error);
            }
            else
                g_warning(_("Samoyed failed to close the pipe connected to the "
                            "stderr of the child process when %s project "
                            "\"%s\" (%s)."),
                          gettext(actionText2[m_action]),
                          m_projectUri.c_str(),
                          m_configName.c_str());
        }
        g_io_channel_unref(m_childStderrChannel);
    }

    if (m_logView)
        m_logViewClosedConn.disconnect();
}

bool BuildJob::run()
{
    // Launch the child process.
    char *pwd = g_filename_from_uri(m_projectUri.c_str(), NULL, NULL);
    char *argv[4];
    gint childStdout, childStderr;
    boost::shared_ptr<char> shell = findShell();
    argv[0] = shell.get();
    argv[1] = strdup("-c");
    argv[2] = strdup(m_commands.c_str());
    argv[3] = NULL;
    GError *error = NULL;
    if (!g_spawn_async_with_pipes(pwd,
                                  argv,
                                  NULL,
                                  G_SPAWN_DO_NOT_REAP_CHILD,
                                  NULL,
                                  NULL,
                                  &m_childPid,
                                  NULL,
                                  &childStdout,
                                  &childStderr,
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
              "(%s)."),
            gettext(actionText[m_action]),
            m_projectUri.c_str(),
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
        g_free(pwd);
        free(argv[1]);
        free(argv[2]);
        return false;
    }
    g_free(pwd);
    free(argv[1]);
    free(argv[2]);
    m_running = true;

    // Start watching the process and the pipes.
    m_childWatchId =
        g_child_watch_add(m_childPid, onChildProcessExited, this);
#ifdef OS_WINDOWS
    m_childStdoutChannel = g_io_channel_win32_new_fd(childStdout);
    m_childStderrChannel = g_io_channel_win32_new_fd(childStderr);
#else
    m_childStdoutChannel = g_io_channel_unix_new(childStdout);
    m_childStderrChannel = g_io_channel_unix_new(childStderr);
#endif
    m_childStdoutWatchId = g_io_add_watch(
        m_childStdoutChannel,
        static_cast<GIOCondition>(G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP |
                                  G_IO_NVAL),
        onChildStdout,
        this);
    m_childStderrWatchId = g_io_add_watch(
        m_childStderrChannel,
        static_cast<GIOCondition>(G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP |
                                  G_IO_NVAL),
        onChildStderr,
        this);
    m_watchingChildStdout = true;
    m_watchingChildStderr = true;

    // Show the build log view.
    BuildLogViewGroup *group =
        Application::instance().currentWindow()->openBuildLogViewGroup();
    m_logView = group->openBuildLogView(m_projectUri.c_str(),
                                        m_configName.c_str());
    m_logView->startBuild(m_action);
    m_logView->clear();
    m_logViewClosedConn = m_logView->addClosedCallback(
        boost::bind(onLogViewClosed, this, _1));

    return true;
}

void BuildJob::stop()
{
    // Stop watching.
    g_source_remove(m_childWatchId);
    if (m_watchingChildStdout)
    {
        g_source_remove(m_childStdoutWatchId);
        m_watchingChildStdout = false;
    }
    if (m_watchingChildStderr)
    {
        g_source_remove(m_childStderrWatchId);
        m_watchingChildStderr = false;
    }

    // Terminate the child process.
#ifdef OS_WINDOWS
    TerminateProcess(m_childPid, 0);
#else
    kill(m_childPid, SIGTERM);
#endif
    g_spawn_close_pid(m_childPid);
    m_running = false;

    if (m_logView)
        m_logView->onBuildStopped();
}

gboolean BuildJob::onFinished(gpointer job)
{
    BuildJob *j = static_cast<BuildJob *>(job);
    if (j->m_logView)
        j->m_logView->onBuildFinished();
    j->m_buildSystem.onBuildFinished(j->m_configName.c_str());
}

void BuildJob::onChildProcessExited(GPid pid, gint status, gpointer job)
{
    BuildJob *j = static_cast<BuildJob *>(job);
    g_spawn_close_pid(j->m_childPid);
    g_source_remove(j->m_childWatchId);
    j->m_running = false;
    if (j->finished())
        g_idle_add(onFinished, job);
}

gboolean BuildJob::onChildStdout(GIOChannel *channel,
                                 GIOCondition condition,
                                 gpointer job)
{
    BuildJob *j = static_cast<BuildJob *>(job);
    if (((condition & G_IO_IN) || (condition & G_IO_PRI)) && j->m_logView)
    {
        gchar *log = NULL;
        gsize length;
        GError *error = NULL;
        GIOStatus status;
        if (condition & G_IO_HUP)
            status = g_io_channel_read_to_end(j->m_childStdoutChannel,
                                              &log,
                                              &length,
                                              &error);
        else
            status = g_io_channel_read_line(j->m_childStdoutChannel,
                                            &log,
                                            &length,
                                            NULL,
                                            &error);
        if (status == G_IO_STATUS_NORMAL && log)
            j->m_logView->addLog(log, length);
        g_free(log);
        if (status == G_IO_STATUS_ERROR)
        {
            if (error)
            {
                g_warning(_("Samoyed failed to read the stdout of the child "
                            "process when %s project \"%s\" (%s): %s."),
                            gettext(actionText2[j->m_action]),
                            j->m_projectUri.c_str(),
                            j->m_configName.c_str(),
                            error->message);
                g_error_free(error);
            }
            else
            {
                g_warning(_("Samoyed failed to read the stdout of the child "
                            "process when %s project \"%s\" (%s)."),
                            gettext(actionText2[j->m_action]),
                            j->m_projectUri.c_str(),
                            j->m_configName.c_str());
            }
        }
    }
    if (condition & G_IO_ERR)
        g_warning(_("Samoyed got a G_IO_ERR error on the stdout of the child "
                    "process when %s project \"%s\" (%s)."),
                  gettext(actionText2[j->m_action]),
                  j->m_projectUri.c_str(),
                  j->m_configName.c_str());
    if (condition & G_IO_NVAL)
        g_warning(_("Samoyed got a G_IO_NVAL error on the stdout of the child "
                    "process when %s project \"%s\" (%s)."),
                  gettext(actionText2[j->m_action]),
                  j->m_projectUri.c_str(),
                  j->m_configName.c_str());
    if (condition & G_IO_HUP)
    {
        j->m_watchingChildStdout = false;
        if (j->finished())
            g_idle_add(onFinished, job);
        return FALSE;
    }
    return TRUE;
}

gboolean BuildJob::onChildStderr(GIOChannel *channel,
                                 GIOCondition condition,
                                 gpointer job)
{
    BuildJob *j = static_cast<BuildJob *>(job);
    if (((condition & G_IO_IN) || (condition & G_IO_PRI)) && j->m_logView)
    {
        gchar *log = NULL;
        gsize length;
        GError *error = NULL;
        GIOStatus status;
        if (condition & G_IO_HUP)
            status = g_io_channel_read_to_end(j->m_childStdoutChannel,
                                              &log,
                                              &length,
                                              &error);
        else
            status = g_io_channel_read_line(j->m_childStdoutChannel,
                                            &log,
                                            &length,
                                            NULL,
                                            &error);
        if (status == G_IO_STATUS_NORMAL && log)
            j->m_logView->addLog(log, length);
        g_free(log);
        if (status == G_IO_STATUS_ERROR)
        {
            if (error)
            {
                g_warning(_("Samoyed failed to read the stderr of the child "
                            "process when %s project \"%s\" (%s): %s."),
                            gettext(actionText2[j->m_action]),
                            j->m_projectUri.c_str(),
                            j->m_configName.c_str(),
                            error->message);
                g_error_free(error);
            }
            else
            {
                g_warning(_("Samoyed failed to read the stderr of the child "
                            "process when %s project \"%s\" (%s)."),
                            gettext(actionText2[j->m_action]),
                            j->m_projectUri.c_str(),
                            j->m_configName.c_str());
            }
        }
    }
    if (condition & G_IO_ERR)
        g_warning(_("Samoyed got a G_IO_ERR error on the stderr of the child "
                    "process when %s project \"%s\" (%s)."),
                  gettext(actionText2[j->m_action]),
                  j->m_projectUri.c_str(),
                  j->m_configName.c_str());
    if (condition & G_IO_NVAL)
        g_warning(_("Samoyed got a G_IO_NVAL error on the stderr of the child "
                    "process when %s project \"%s\" (%s)."),
                  gettext(actionText2[j->m_action]),
                  j->m_projectUri.c_str(),
                  j->m_configName.c_str());
    if (condition & G_IO_HUP)
    {
        j->m_watchingChildStderr = false;
        if (j->finished())
            g_idle_add(onFinished, job);
        return FALSE;
    }
    return TRUE;
}

void BuildJob::onLogViewClosed(Widget &widget)
{
    m_logView = NULL;
}

}
