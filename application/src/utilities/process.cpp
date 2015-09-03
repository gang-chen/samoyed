// Process.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "process.hpp"
#ifdef OS_WINDOWS
# include <windows.h>
#else
# include <sys/types.h>
# include <signal.h>
#endif
#include <glib.h>

namespace Samoyed
{

Process::Process():
    m_running(false),
    m_watchingStdin(false),
    m_watchingStdout(false),
    m_watchingStderr(false),
    m_stdinChannel(NULL),
    m_stdoutChannel(NULL),
    m_stderrChannel(NULL)
{
}

Process::~Process()
{
    if (running())
        stop();
    if (m_stdinChannel)
        closeStdin(NULL);
    if (m_stdoutChannel)
        closeStdout(NULL);
    if (m_stderrChannel)
        closeStderr(NULL);
}

bool Process::run(
    const char *workDir,
    const char **argv,
    const char **envp,
    const boost::function<void (Process &, int)> &onFinished,
    const boost::function<void (Process &, GIOChannel *, int)> &onStdin,
    const boost::function<void (Process &, GIOChannel *, int)> &onStdout,
    const boost::function<void (Process &, GIOChannel *, int)> &onStderr,
    GError **error)
{
    // Launch the process.
    char **argv2 = g_strdupv(const_cast<char **>(argv));
    char **envp2 = g_strdupv(const_cast<char **>(envp));
    gint childStdin, childStdout, childStderr;
    if (!g_spawn_async_with_pipes(
            workDir,
            argv2,
            envp2,
            G_SPAWN_DO_NOT_REAP_CHILD,
            NULL,
            NULL,
            &m_pid,
            onStdin ? &childStdin : NULL,
            onStdout ? &childStdout : NULL,
            onStderr ? &childStderr : NULL,
            error))
    {
        g_strfreev(argv2);
        g_strfreev(envp2);
        return false;
    }
    g_strfreev(argv2);
    g_strfreev(envp2);
    m_running = true;

    // Start watching the process and the pipes.
    m_exitWatchId = g_child_watch_add(m_pid, onExited, this);
    if (onStdin)
    {
        m_onStdin = onStdin;
#ifdef OS_WINDOWS
        m_stdinChannel = g_io_channel_win32_new_fd(childStdin);
#else
        m_stdinChannel = g_io_channel_unix_new(childStdin);
#endif
        m_stdinWatchId = g_io_add_watch(
            m_stdinChannel,
            static_cast<GIOCondition>(
                G_IO_OUT | G_IO_ERR | G_IO_HUP | G_IO_NVAL),
            Process::onStdin,
            this);
        m_watchingStdin = true;
    }
    if (onStdout)
    {
        m_onStdout = onStdout;
#ifdef OS_WINDOWS
        m_stdoutChannel = g_io_channel_win32_new_fd(childStdout);
#else
        m_stdoutChannel = g_io_channel_unix_new(childStdout);
#endif
        m_stdoutWatchId = g_io_add_watch(
            m_stdoutChannel,
            static_cast<GIOCondition>(
                G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL),
            Process::onStdout,
            this);
        m_watchingStdout = true;
    }
    if (onStderr)
    {
        m_onStderr = onStderr;
#ifdef OS_WINDOWS
        m_stderrChannel = g_io_channel_win32_new_fd(childStderr);
#else
        m_stderrChannel = g_io_channel_unix_new(childStderr);
#endif
        m_stderrWatchId = g_io_add_watch(
            m_stderrChannel,
            static_cast<GIOCondition>(
                G_IO_IN | G_IO_PRI | G_IO_ERR | G_IO_HUP | G_IO_NVAL),
            Process::onStderr,
            this);
        m_watchingStderr = true;
    }
    return true;
}

void Process::stop()
{
    // Stop watching.
    g_source_remove(m_exitWatchId);
    if (m_watchingStdin)
    {
        g_source_remove(m_stdinWatchId);
        m_watchingStdin = false;
    }
    if (m_watchingStdout)
    {
        g_source_remove(m_stdoutWatchId);
        m_watchingStdout = false;
    }
    if (m_watchingStderr)
    {
        g_source_remove(m_stderrWatchId);
        m_watchingStderr = false;
    }

    // Terminate the process.
#ifdef OS_WINDOWS
    TerminateProcess(m_pid, 0);
#else
    kill(m_pid, SIGTERM);
#endif
    g_spawn_close_pid(m_pid);
    m_running = false;
}

GIOStatus Process::closeStdin(GError **error)
{
    if (!m_stdinChannel)
        return G_IO_STATUS_NORMAL;
    if (m_watchingStdin)
    {
        g_source_remove(m_stdinWatchId);
        m_watchingStdin = false;
    }
    GIOStatus status = g_io_channel_shutdown(m_stdinChannel, TRUE, error);
    g_io_channel_unref(m_stdinChannel);
    m_stdinChannel = NULL;
    return status;
}

GIOStatus Process::closeStdout(GError **error)
{
    if (!m_stdoutChannel)
        return G_IO_STATUS_NORMAL;
    if (m_watchingStdout)
    {
        g_source_remove(m_stdoutWatchId);
        m_watchingStdout = false;
    }
    GIOStatus status = g_io_channel_shutdown(m_stdoutChannel, TRUE, error);
    g_io_channel_unref(m_stdoutChannel);
    m_stdoutChannel = NULL;
    return status;
}

GIOStatus Process::closeStderr(GError **error)
{
    if (!m_stderrChannel)
        return G_IO_STATUS_NORMAL;
    if (m_watchingStderr)
    {
        g_source_remove(m_stderrWatchId);
        m_watchingStderr = false;
    }
    GIOStatus status = g_io_channel_shutdown(m_stderrChannel, TRUE, error);
    g_io_channel_unref(m_stderrChannel);
    m_stderrChannel = NULL;
    return status;
}

gboolean Process::onFinished(gpointer process)
{
    Process *proc = static_cast<Process *>(process);
    proc->m_onFinished(*proc, proc->m_exitCode);
}

void Process::onExited(GPid pid, gint status, gpointer process)
{
    Process *proc = static_cast<Process *>(process);
    g_spawn_close_pid(proc->m_pid);
    g_source_remove(proc->m_exitWatchId);
    proc->m_running = false;
    proc->m_exitCode = status;
    if (proc->finished() && proc->m_onFinished)
        g_idle_add(onFinished, process);
}

gboolean Process::onStdin(GIOChannel *channel,
                          GIOCondition condition,
                          gpointer process)
{
    Process *proc = static_cast<Process *>(process);
    proc->m_onStdin(*proc, channel, condition);
    if (condition & G_IO_HUP)
    {
        proc->m_watchingStdin = false;
        if (proc->finished() && proc->m_onFinished)
            g_idle_add(onFinished, process);
        return FALSE;
    }
    return TRUE;
}

gboolean Process::onStdout(GIOChannel *channel,
                           GIOCondition condition,
                           gpointer process)
{
    Process *proc = static_cast<Process *>(process);
    proc->m_onStdout(*proc, channel, condition);
    if (condition & G_IO_HUP)
    {
        proc->m_watchingStdout = false;
        if (proc->finished() && proc->m_onFinished)
            g_idle_add(onFinished, process);
        return FALSE;
    }
    return TRUE;
}

gboolean Process::onStderr(GIOChannel *channel,
                           GIOCondition condition,
                           gpointer process)
{
    Process *proc = static_cast<Process *>(process);
    proc->m_onStderr(*proc, channel, condition);
    if (condition & G_IO_HUP)
    {
        proc->m_watchingStderr = false;
        if (proc->finished() && proc->m_onFinished)
            g_idle_add(onFinished, process);
        return FALSE;
    }
    return TRUE;
}

}
