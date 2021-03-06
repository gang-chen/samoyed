// Builder.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILDER_HPP
#define SMYD_BUILDER_HPP

#include <boost/utility.hpp>
#include <boost/signals2/signal.hpp>
#include <glib.h>
#include <gio/gio.h>

namespace Samoyed
{

class BuildSystem;
class BuildLogView;
class Configuration;
class Widget;

class Builder: public boost::noncopyable
{
public:
    enum Action
    {
        ACTION_CONFIGURE,
        ACTION_BUILD,
        ACTION_INSTALL,
        ACTION_CLEAN
    };

    Builder(BuildSystem &buildSystem,
            Configuration &config,
            Action action);

    ~Builder();

    bool running() const;

    bool run();

    void stop();

private:
    static void onProcessExited(GPid processId,
                                gint status,
                                gpointer builder);

    static void onDataRead(GObject *stream,
                           GAsyncResult *result,
                           gpointer param);

    void onLogViewClosed(Widget &widget);

    void onFinished();

    BuildSystem &m_buildSystem;
    Configuration &m_configuration;
    Action m_action;

    BuildLogView *m_logView;
    boost::signals2::connection m_logViewClosedConn;

    bool m_processRunning;
    GPid m_processId;
    guint m_processWatchId;
    GInputStream *m_outputPipe;
    GCancellable *m_cancelReadingOutput;

#ifdef OS_WINDOWS
    bool m_usingWindowsCmd;
#endif
};

}

#endif
