// Process.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROCESS_HPP
#define SMYD_PROCESS_HPP

#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <glib.h>

namespace Samoyed
{

class Process: public boost::noncopyable
{
public:
    Process();

    ~Process();

    bool run(
        const char *workDir,
        const char **argv,
        const char **envp,
        const boost::function<void (Process &, int)> &onFinished,
        const boost::function<void (Process &, GIOChannel *, int)> &onStdin,
        const boost::function<void (Process &, GIOChannel *, int)> &onStdout,
        const boost::function<void (Process &, GIOChannel *, int)> &onStderr,
        GError **error);

    bool running() const { return m_running; }

    void stop();

    GIOStatus closeStdin(GError **error);
    GIOStatus closeStdout(GError **error);
    GIOStatus closeStderr(GError **error);

private:
    bool finished() const
    { return !m_running && !m_watchingStdout && !m_watchingStderr; }

    static gboolean onFinished(gpointer process);

    static void onExited(GPid pid, gint status, gpointer process);

    static gboolean onStdin(GIOChannel *channel,
                            GIOCondition condition,
                            gpointer process);

    static gboolean onStdout(GIOChannel *channel,
                             GIOCondition condition,
                             gpointer process);

    static gboolean onStderr(GIOChannel *channel,
                             GIOCondition condition,
                             gpointer process);

    bool m_running;
    GPid m_pid;
    guint m_exitWatchId;
    int m_exitCode;

    bool m_watchingStdin;
    bool m_watchingStdout;
    bool m_watchingStderr;
    GIOChannel *m_stdinChannel;
    GIOChannel *m_stdoutChannel;
    GIOChannel *m_stderrChannel;
    guint m_stdinWatchId;
    guint m_stdoutWatchId;
    guint m_stderrWatchId;

    boost::function<void (Process &, int)> m_onFinished;
    boost::function<void (Process &, GIOChannel *, int)> m_onStdin;
    boost::function<void (Process &, GIOChannel *, int)> m_onStdout;
    boost::function<void (Process &, GIOChannel *, int)> m_onStderr;
};

}

#endif
