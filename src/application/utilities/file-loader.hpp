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
    FileLoader(unsigned int priority,
               const Callback &callback,
               const char *uri):
        Worker(priority, callback),
        m_uri(uri)
    {}

    const Revision &revision() const { return m_revision; }

    GError *fetchError()
    {
        GError *error = m_error;
        m_error = NULL;
        return error;
    }

protected:
private:
    const std::string m_uri;
    Revision m_revision;
    GError *m_error;
};

}

#endif
