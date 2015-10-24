// Compiler options collector.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_COMPILER_OPTIONS_COLLECTOR_HPP
#define SMYD_COMPILER_OPTIONS_COLLECTOR_HPP

#include "utilities/worker.hpp"
#include <string>

namespace Samoyed
{

class CompilerOptionsCollector: public Worker
{
public:
    CompilerOptionsCollector(Scheduler &scheduler,
                             unsigned int priority,
                             const char *projectUri,
                             const char *configName,
                             const char *inputFileName);

protected:
    virtual bool step();

private:
    std::string m_projectUri;
    std::string m_configName;
    std::string m_inputFileName;
};

}

#endif
