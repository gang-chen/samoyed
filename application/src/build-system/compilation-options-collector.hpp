// Compilation options collector.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_COMPILATION_OPTIONS_COLLECTOR_HPP
#define SMYD_COMPILATION_OPTIONS_COLLECTOR_HPP

#include <string>
#include <boost/utility.hpp>
#include <glib.h>
#include <gio/gio.h>

namespace Samoyed
{

class BuildSystem;

class CompilationOptionsCollector: public boost::noncopyable
{
public:
    CompilationOptionsCollector(BuildSystem &buildSystem,
                                const char *commands);

    ~CompilationOptionsCollector();

    bool running() const;

    bool run();

    void stop();

private:
    static void onProcessExited(GPid processId,
                                gint status,
                                gpointer collector);

    static void onDataRead(GObject *stream,
                           GAsyncResult *result,
                           gpointer param);

    BuildSystem &m_buildSystem;
    std::string m_commands;

    bool m_processRunning;
    GPid m_processId;
    guint m_processWatchId;
    GInputStream *m_stdoutPipe;
    GCancellable *m_cancelReadingStdout;
};

}

#endif
