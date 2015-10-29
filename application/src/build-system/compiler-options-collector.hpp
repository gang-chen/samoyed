// Compiler options collector.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_COMPILER_OPTIONS_COLLECTOR_HPP
#define SMYD_COMPILER_OPTIONS_COLLECTOR_HPP

#include "utilities/worker.hpp"
#include <string>
#include <gio/gio.h>

namespace Samoyed
{

class Project;
class ProjectDb;

class CompilerOptionsCollector: public Worker
{
public:
    CompilerOptionsCollector(Scheduler &scheduler,
                             unsigned int priority,
                             Project &project,
                             const char *inputFileName);

    virtual ~CompilerOptionsCollector();

protected:
    virtual bool step();

private:
    bool parse(const char *&begin, const char *end);

    ProjectDb &m_projectDb;

    std::string m_inputFileName;

    GInputStream *m_stream;
    char *m_readBuffer;
    char *m_readPointer;
};

}

#endif
