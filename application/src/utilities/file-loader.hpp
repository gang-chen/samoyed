// File loader.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_LOADER_HPP
#define SMYD_FILE_LOADER_HPP

#include "worker.hpp"
#include "miscellaneous.hpp"
#include <string>
#include <glib.h>

namespace Samoyed
{

class FileLoader: public Worker
{
public:
    FileLoader(Scheduler &scheduler,
               unsigned int priority,
               const char *uri):
        Worker(scheduler, priority),
        m_error(NULL),
        m_uri(uri)
    {}

    virtual ~FileLoader()
    {
        if (m_error)
            g_error_free(m_error);
    }

    const char *uri() const { return m_uri.c_str(); }

    const Time &modifiedTime() const { return m_modifiedTime; }

    const GError *error() const { return m_error; }

protected:
    Time m_modifiedTime;
    GError *m_error;

private:
    const std::string m_uri;
};

}

#endif
