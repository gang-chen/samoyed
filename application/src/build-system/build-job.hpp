// Build job.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_JOB_HPP
#define SMYD_BUILD_JOB_HPP

#include <string>
#include <boost/utility.hpp>
#include <boost/signals2/signal.hpp>
#include <glib.h>

namespace Samoyed
{

class BuildSystem;
class BuildLogView;
class Widget;

class BuildJob: public boost::noncopyable
{
public:
    enum Action
    {
        ACTION_CONFIGURE,
        ACTION_BUILD,
        ACTION_INSTALL
    };

    BuildJob(BuildSystem &buildSystem,
             const char *commands,
             Action action,
             const char *projectUri,
             const char *configName);

    ~BuildJob();

    bool run();

    bool running() const { return m_running; }

    void stop();

private:
    bool finished() const
    { return !m_running && !m_watchingChildStdout && !m_watchingChildStderr; }

    static gboolean onFinished(gpointer job);

    static void onChildProcessExited(GPid pid, gint status, gpointer job);

    static gboolean onChildStdout(GIOChannel *channel,
                                  GIOCondition condition,
                                  gpointer job);

    static gboolean onChildStderr(GIOChannel *channel,
                                  GIOCondition condition,
                                  gpointer job);

    void onLogViewClosed(Widget &widget);

    BuildSystem &m_buildSystem;
    std::string m_commands;
    Action m_action;
    std::string m_projectUri;
    std::string m_configName;

    bool m_running;
    GPid m_childPid;
    guint m_childWatchId;

    bool m_watchingChildStdout;
    bool m_watchingChildStderr;
    GIOChannel *m_childStdoutChannel;
    GIOChannel *m_childStderrChannel;
    guint m_childStdoutWatchId;
    guint m_childStderrWatchId;

    BuildLogView *m_logView;
    boost::signals2::connection m_logViewClosedConn;
};

}

#endif
