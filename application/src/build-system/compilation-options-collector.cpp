// Compilation options collector.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "compilation-options-collector.hpp"
#include "build-system.hpp"
#include "project/project.hpp"
#include <glib.h>
#include <glib/gi18n.h>

namespace Samoyed
{

CompilationOptionsCollector::CompilationOptionsCollector(
    BuildSystem &buildSystem,
    const char *commands):
        m_buildSystem(buildSystem),
        m_commands(commands)
{
}

CompilationOptionsCollector::~CompilationOptionsCollector()
{
    if (running())
        stop();

    GError *error;
    GIOStatus status;
    error = NULL;
    status = closeStdout(&error);
    if (status != G_IO_STATUS_NORMAL)
    {
        if (error)
        {
            g_warning(_("Samoyed failed to close the pipe connected to the "
                        "stdout of the child process when collecting "
                        "compilation options for project \"%s\": %s."),
                      m_buildSystem.project().uri(),
                      error->message);
            g_error_free(error);
        }
        else
            g_warning(_("Samoyed failed to close the pipe connected to the "
                        "stdout of the child process when collecting "
                        "compilation options for project \"%s\"."),
                      m_buildSystem.project().uri());
    }
}

bool CompilationOptionsCollector::run()
{
    char *pwd = g_filename_from_uri(m_buildSystem.project().uri(), NULL, NULL);
    const char *argv[4];
    boost::shared_ptr<char> shell = findShell();
    argv[0] = shell.get();
    argv[1] = "-c";
    argv[2] = m_commands.c_str();
    argv[3] = NULL;
    GError *error = NULL;
    if (!Process::run(pwd,
                      argv,
                      NULL,
                      onFinished,
                      boost::function<void (Process &, GIOChannel *, int)>(),
                      onStdout,
                      boost::function<void (Process &, GIOChannel *, int)>(),
                      &error))
    {
        if (error)
        {
            g_warning(_("Samoyed failed to start a child process to collect "
                        "compilation options for project \"%s\": %s."),
                      m_buildSystem.project().uri(),
                      error->message);
            g_error_free(error);
        }
        else
            g_warning(_("Samoyed failed to start a child process to collect "
                        "compilation options for project \"%s\"."),
                      m_buildSystem.project().uri());
        return false;
    }
    return true;
}

void CompilationOptionsCollector::onFinished(Process &process, int exitCode)
{
    static_cast<CompilationOptionsCollector &>(process).m_buildSystem.
        onCompilationOptionsCollectorFinished();
}

void CompilationOptionsCollector::onStdout(Process &process,
                                           GIOChannel *channel,
                                           int condition)
{
    CompilationOptionsCollector &coll =
        static_cast<CompilationOptionsCollector &>(process);
    if ((condition & G_IO_IN) || (condition & G_IO_PRI))
    {
    }
}

}
