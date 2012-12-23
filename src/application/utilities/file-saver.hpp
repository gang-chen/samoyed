// File saver.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_SAVER_HPP
#define SMYD_FILE_SAVER_HPP

#include "worker.hpp"
#include "revision.hpp"
#include <string>
#include <glib.h>

namespace Samoyed
{

class FileSaver: public Worker
{
public:
    FileSaver(unsigned int priority,
              const Callback &callback,
              const char *uri):
        Worker(priority, callback),
        m_uri(uri),
        m_error(NULL)
    {}

    virtual ~FileSaver()
    {
        if (m_error)
            g_error_free(m_error);
    }

    const char *uri() const { return m_uri.c_str(); }

    const Revision &revision() const { return m_revision; }

    GError *takeError()
    {
        GError *error = m_error;
        m_error = NULL;
        return error;
    }

protected:
    Revision &revision() { return m_revision; }

    GError *&error() { return m_error; }

private:
    const std::string m_uri;
    Revision m_revision;
    GError *m_error;
};

}

#endif
