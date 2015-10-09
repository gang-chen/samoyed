// Compilation options collector.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "compilation-options-collector.hpp"
#include "build-system.hpp"
#include "utilities/miscellaneous.hpp"
#include "project/project.hpp"
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

struct DataReadParam
{
    GCancellable *cancellable;
    Samoyed::CompilationOptionsCollector &collector;
    char buffer[BUFSIZ];
    DataReadParam(GCancellable *cancel,
                  Samoyed::CompilationOptionsCollector &coll):
        cancellable(cancel),
        collector(coll)
    { g_object_ref(cancellable); }
    ~DataReadParam()
    { g_object_unref(cancellable); }
};

}

namespace Samoyed
{

CompilationOptionsCollector::CompilationOptionsCollector(
    BuildSystem &buildSystem,
    const char *commands):
        m_buildSystem(buildSystem),
        m_commands(commands),
        m_processRunning(false),
        m_stdoutPipe(NULL),
        m_cancelReadingStdout(NULL)
{
}

CompilationOptionsCollector::~CompilationOptionsCollector()
{
    if (running())
        stop();
}

bool CompilationOptionsCollector::running() const
{
    return m_processRunning || m_stdoutPipe;
}

bool CompilationOptionsCollector::run()
{
    char *cwd = g_filename_from_uri(m_buildSystem.project().uri(), NULL, NULL);
    char *argv[5] = { NULL, NULL, NULL, NULL, NULL };
    char **env = NULL;
#ifdef OS_WINDOWS
    char *instDir =
        g_win32_get_package_installation_directory_of_module(NULL);
    argv[0] = g_strconcat(instDir, "\\bin\\bash.exe", NULL);
    argv[1] = g_strdup("-l");
    argv[2] = g_strdup("-c");
    argv[3] = g_strdup(m_commands.c_str());
    g_free(instDir);
    if (!g_getenv("MSYSTEM"))
    {
        env = g_get_environ();
        env = g_environ_setenv(env, "MSYSTEM", "MINGW32", FALSE);
    }
#else
    const char *shell = g_getenv("SHELL");
    if (shell)
        argv[0] = g_strdup(shell);
    else
    {
        struct passwd *pw;
        pw = getpwuid(getuid());
        if (pw)
            argv[0] = g_strdup(pw->pw_shell);
        else if (g_file_test("/usr/bin/bash", G_FILE_TEST_IS_EXECUTABLE))
            argv[0] = g_strdup("/usr/bin/bash");
        else
            argv[0] = g_strdup("/bin/sh");
    }
    argv[1] = g_strdup("-c");
    argv[2] = g_strdup(m_commands.c_str());
#endif
    GError *error = NULL;
    if (!spawnSubprocess(cwd,
                         argv,
                         env,
                         SPAWN_SUBPROCESS_FLAG_STDOUT_PIPE,
                         &m_processId,
                         NULL,
                         &m_stdoutPipe,
                         NULL,
                         &error))
    {
        g_warning(_("Samoyed failed to start a child process to collect "
                    "compilation options for project \"%s\"."),
                  m_buildSystem.project().uri());
        g_error_free(error);
        g_free(cwd);
        g_free(argv[0]);
        g_free(argv[1]);
        g_free(argv[2]);
        g_free(argv[3]);
        g_strfreev(env);
        return false;
    }
    g_free(cwd);
    g_free(argv[0]);
    g_free(argv[1]);
    g_free(argv[2]);
    g_free(argv[3]);
    g_strfreev(env);

    m_processRunning = true;
    m_processWatchId = g_child_watch_add(m_processId, onProcessExited, this);

    m_cancelReadingStdout = g_cancellable_new();
    DataReadParam *param = new DataReadParam(m_cancelReadingStdout, *this);
    g_input_stream_read_async(
        m_stdoutPipe,
        param->buffer,
        BUFSIZ,
        G_PRIORITY_DEFAULT,
        m_cancelReadingStdout,
        onDataRead,
        param);

    return true;
}

void CompilationOptionsCollector::stop()
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
    if (m_stdoutPipe)
    {
        g_cancellable_cancel(m_cancelReadingStdout);
        g_object_unref(m_cancelReadingStdout);
        m_cancelReadingStdout = NULL;
        g_object_unref(m_stdoutPipe);
        m_stdoutPipe = NULL;
    }
}

void CompilationOptionsCollector::onProcessExited(GPid processId,
                                                  gint status,
                                                  gpointer collector)
{
    CompilationOptionsCollector *c =
        static_cast<CompilationOptionsCollector *>(collector);
    g_source_remove(c->m_processWatchId);
    g_spawn_close_pid(c->m_processId);
    c->m_processRunning = false;

    if (!c->m_stdoutPipe)
        c->m_buildSystem.onCompilationOptionsCollectionFinished();
}

void CompilationOptionsCollector::onDataRead(GObject *stream,
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
                g_warning(_("Samoyed failed to read the stdout of the child "
                            "process when collecting compilation options for "
                            "project \"%s\": %s."),
                       p->collector.m_buildSystem.project().uri(),
                       error->message);
                g_error_free(error);
            }

            g_object_unref(p->collector.m_cancelReadingStdout);
            p->collector.m_cancelReadingStdout = NULL;
            g_object_unref(p->collector.m_stdoutPipe);
            p->collector.m_stdoutPipe = NULL;

            if (!p->collector.m_processRunning)
                p->collector.m_buildSystem.
                    onCompilationOptionsCollectionFinished();
            delete p;
        }
    }
    else
        delete p;
}

}
