// File loader.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_LOADER_HPP
#define SMYD_FILE_LOADER_HPP

#include "worker.hpp"
#include "revision.hpp"
#include <string>
#include <glib.h>

namespace Samoyed
{

class FileLoader: public Worker
{
public:
    FileLoader(Scheduler &scheduler,
               unsigned int priority,
               const Callback &callback,
               const char *uri):
        Worker(scheduler, priority, callback),
        m_error(NULL),
        m_uri(uri)
    {}

    virtual ~FileLoader()
    {
        if (m_error)
            g_error_free(m_error);
    }

    const char *uri() const { return m_uri.c_str(); }

    const Revision &revision() const { return m_revision; }

    GError *error() const { return m_error; }

    GError *takeError()
    {
        GError *error = m_error;
        m_error = NULL;
        return error;
    }

protected:
    Revision m_revision;
    GError *m_error;

private:
    const std::string m_uri;
};

}

#endif
