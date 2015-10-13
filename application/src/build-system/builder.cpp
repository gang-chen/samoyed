// Builder.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "builder.hpp"
#include "build-system.hpp"
#include "build-log-view.hpp"
#include "build-log-view-group.hpp"
#include "configuration.hpp"
#include "utilities/miscellaneous.hpp"
#include "project/project.hpp"
#include "window/window.hpp"
#include "application.hpp"
#ifdef OS_WINDOWS
# include <windows.h>
#else
# include <pwd.h>
# include <unistd.h>
# include <sys/types.h>
# include <signal.h>
#endif
#include <glib.h>
#include <gio/gio.h>
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

struct DataReadParam
{
    GCancellable *cancellable;
    Samoyed::Builder &builder;
    char buffer[BUFSIZ];
    DataReadParam(GCancellable *c,
                  Samoyed::Builder &b):
        cancellable(c),
        builder(b)
    { g_object_ref(cancellable); }
    ~DataReadParam()
    { g_object_unref(cancellable); }
};

}

namespace Samoyed
{

Builder::Builder(BuildSystem &buildSystem,
                 Configuration &config,
                 Action action):
    m_buildSystem(buildSystem),
    m_configuration(config),
    m_action(action),
    m_logView(NULL),
    m_processRunning(false),
    m_outputPipe(NULL),
    m_cancelReadingOutput(NULL)
{
}

Builder::~Builder()
{
    if (running())
        stop();

    if (m_logView)
        m_logViewClosedConn.disconnect();
}

bool Builder::running() const
{
    return m_processRunning || m_outputPipe;
}

bool Builder::run()
{
    const char *commands;
    switch (m_action)
    {
    case ACTION_CONFIGURE:
        commands = m_configuration.configureCommands();
        break;
    case ACTION_BUILD:
        commands = m_configuration.buildCommands();
        break;
    case ACTION_INSTALL:
        commands = m_configuration.installCommands();
        break;
    }

    char *cwd = g_filename_from_uri(m_buildSystem.project().uri(), NULL, NULL);
    const char *argv[5] = { NULL, NULL, NULL, NULL, NULL };
    char **envv = NULL;
#ifdef OS_WINDOWS
    char *instDir =
        g_win32_get_package_installation_directory_of_module(NULL);
    char *arg0;
    arg0 = g_strconcat(instDir, "\\bin\\bash.exe", NULL);
    argv[0] = arg0;
    argv[1] = "-l";
    argv[2] = "-c";
    argv[3] = commands;
    g_free(instDir);
    if (!g_getenv("MSYSTEM"))
    {
        envv = g_get_environ();
        envv = g_environ_setenv(envv, "MSYSTEM", "MINGW32", FALSE);
    }
#else
    const char *shell = g_getenv("SHELL");
    if (shell)
        argv[0] = shell;
    else
    {
        struct passwd *pw;
        pw = getpwuid(getuid());
        if (pw)
            argv[0] = pw->pw_shell;
        else if (g_file_test("/usr/bin/bash", G_FILE_TEST_IS_EXECUTABLE))
            argv[0] = "/usr/bin/bash";
        else
            argv[0] = "/bin/sh";
    }
    argv[1] = "-c";
    argv[2] = commands;
#endif
    GError *error = NULL;
    if (!spawnSubprocess(cwd,
                         argv,
                         const_cast<const char **>(envv),
                         SPAWN_SUBPROCESS_FLAG_STDOUT_PIPE |
                         SPAWN_SUBPROCESS_FLAG_STDERR_MERGE,
                         &m_processId,
                         NULL,
                         &m_outputPipe,
                         NULL,
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
            m_configuration.name());
        gtkMessageDialogAddDetails(dialog, _("%s."), error->message);
        g_error_free(error);
        g_free(cwd);
#ifdef OS_WINDOWS
        g_free(arg0);
#endif
        g_strfreev(envv);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }
    g_free(cwd);
#ifdef OS_WINDOWS
    g_free(arg0);
#endif
    g_strfreev(envv);

    m_processRunning = true;
    m_processWatchId = g_child_watch_add(m_processId, onProcessExited, this);

    m_cancelReadingOutput = g_cancellable_new();
    DataReadParam *param = new DataReadParam(m_cancelReadingOutput, *this);
    g_input_stream_read_async(
        m_outputPipe,
        param->buffer,
        BUFSIZ,
        G_PRIORITY_DEFAULT,
        m_cancelReadingOutput,
        onDataRead,
        param);

    // Show the build log view.
    BuildLogViewGroup *group =
        Application::instance().currentWindow()->openBuildLogViewGroup();
    m_logView = group->openBuildLogView(m_buildSystem.project().uri(),
                                        m_configuration.name());
    m_logView->startBuild(m_action);
    m_logView->clear();
    m_logViewClosedConn = m_logView->addClosedCallback(
        boost::bind(onLogViewClosed, this, _1));

    return true;
}

void Builder::stop()
{
    if (m_processRunning)
    {
        g_source_remove(m_processWatchId);
#ifdef OS_WINDOWS
        TerminateProcess(m_processId, 1);
#else
        kill(m_processId, SIGKILL);
#endif
        g_spawn_close_pid(m_processId);
        m_processRunning = false;
    }
    if (m_outputPipe)
    {
        g_cancellable_cancel(m_cancelReadingOutput);
        g_object_unref(m_cancelReadingOutput);
        m_cancelReadingOutput = NULL;
        g_object_unref(m_outputPipe);
        m_outputPipe = NULL;
    }
    if (m_logView)
        m_logView->onBuildStopped();
}

void Builder::onProcessExited(GPid processId,
                              gint status,
                              gpointer builder)
{
    Builder *b = static_cast<Builder *>(builder);
    g_source_remove(b->m_processWatchId);
    g_spawn_close_pid(b->m_processId);
    b->m_processRunning = false;

    if (b->m_logView)
    {
        GError *error = NULL;
        b->m_logView->onBuildFinished(g_spawn_check_exit_status(status, &error),
                                      error ? error->message : NULL);
        if (error)
            g_error_free(error);
    }

    if (!b->m_outputPipe)
        b->m_buildSystem.onBuildFinished(b->m_configuration.name());
}

void Builder::onDataRead(GObject *stream,
                         GAsyncResult *result,
                         gpointer param)
{
    DataReadParam *p = static_cast<DataReadParam *>(param);
    if (!g_cancellable_is_cancelled(p->cancellable))
    {
        GError *error = NULL;
        int length = g_input_stream_read_finish(G_INPUT_STREAM(stream),
                                                result,
                                                &error);
        if (length > 0)
        {
            if (p->builder.m_logView)
                p->builder.m_logView->addLog(p->buffer, length);
            g_input_stream_read_async(G_INPUT_STREAM(stream),
                                      p->buffer,
                                      BUFSIZ,
                                      G_PRIORITY_DEFAULT,
                                      p->cancellable,
                                      onDataRead,
                                      p);
        }
        else
        {
            if (length == -1)
            {
                g_warning(_("Samoyed failed to read the stdout or stderr of "
                            "the child process when %s project \"%s\" with "
                            "configuration \"%s\": %s."),
                       gettext(actionText2[p->builder.m_action]),
                       p->builder.m_buildSystem.project().uri(),
                       p->builder.m_configuration.name(),
                       error->message);
                g_error_free(error);
            }

            g_object_unref(p->builder.m_cancelReadingOutput);
            p->builder.m_cancelReadingOutput = NULL;
            g_object_unref(p->builder.m_outputPipe);
            p->builder.m_outputPipe = NULL;

            if (!p->builder.m_processRunning)
                p->builder.m_buildSystem.onBuildFinished(
                    p->builder.m_configuration.name());
            delete p;
        }
    }
    else
        delete p;
}

void Builder::onLogViewClosed(Widget &widget)
{
    m_logView = NULL;
}

}
