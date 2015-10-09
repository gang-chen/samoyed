// Builder.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILDER_HPP
#define SMYD_BUILDER_HPP

#include <string>
#include <boost/utility.hpp>
#include <boost/signals2/signal.hpp>
#include <glib.h>
#include <gio/gio.h>

namespace Samoyed
{

class BuildSystem;
class BuildLogView;
class Widget;

class Builder: public boost::noncopyable
{
public:
    enum Action
    {
        ACTION_CONFIGURE,
        ACTION_BUILD,
        ACTION_INSTALL
    };

    Builder(BuildSystem &buildSystem,
            const char *configName,
            const char *commands,
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

    BuildSystem &m_buildSystem;
    std::string m_configName;
    std::string m_commands;
    Action m_action;

    BuildLogView *m_logView;
    boost::signals2::connection m_logViewClosedConn;

    bool m_processRunning;
    GPid m_processId;
    guint m_processWatchId;
    GInputStream *m_outputPipe;
    GCancellable *m_cancelReadingOutput;
};

}

#endif
