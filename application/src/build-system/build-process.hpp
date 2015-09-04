// Build process.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_PROCESS_HPP
#define SMYD_BUILD_PROCESS_HPP

#include "utilities/process.hpp"
#include <string>
#include <boost/utility.hpp>
#include <boost/signals2/signal.hpp>
#include <glib.h>

namespace Samoyed
{

class BuildSystem;
class BuildLogView;
class Widget;

class BuildProcess: public Process
{
public:
    enum Action
    {
        ACTION_CONFIGURE,
        ACTION_BUILD,
        ACTION_INSTALL
    };

    BuildProcess(BuildSystem &buildSystem,
                 const char *configName,
                 const char *commands,
                 Action action);

    ~BuildProcess();

    bool run();

    void stop();

private:
    static void onFinished(Process &process, int exitCode);

    static void onStdout(Process &process,
                         GIOChannel *channel,
                         int condition);

    static void onStderr(Process &process,
                         GIOChannel *channel,
                         int condition);

    void onLogViewClosed(Widget &widget);

    BuildSystem &m_buildSystem;
    std::string m_configName;
    std::string m_commands;
    Action m_action;

    BuildLogView *m_logView;
    boost::signals2::connection m_logViewClosedConn;
};

}

#endif
