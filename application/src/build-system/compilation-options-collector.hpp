// Compilation options collector.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_COMPILATION_OPTIONS_COLLECTOR_HPP
#define SMYD_COMPILATION_OPTIONS_COLLECTOR_HPP

#include "utilities/process.hpp"
#include <string>

namespace Samoyed
{

class BuildSystem;

class CompilationOptionsCollector: public Process
{
public:
    CompilationOptionsCollector(BuildSystem &buildSystem,
                                const char *commands);

    ~CompilationOptionsCollector();

    bool run();

private:
    static void onFinished(Process &process, int exitCode);

    static void onStdout(Process &process,
                         GIOChannel *channel,
                         int condition);

    BuildSystem &m_buildSystem;
    std::string m_commands;
};

}

#endif
