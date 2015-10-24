// Compiler options collector.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "compiler-options-collector.hpp"

namespace Samoyed
{

CompilerOptionsCollector::CompilerOptionsCollector(
    Scheduler &scheduler,
    unsigned int priority,
    const char *projectUri,
    const char *configName,
    const char *inputFileName):
        Worker(scheduler, priority),
        m_projectUri(projectUri),
        m_configName(configName),
        m_inputFileName(inputFileName)
{
}

bool CompilerOptionsCollector::step()
{
}

}
