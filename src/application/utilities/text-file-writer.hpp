// Text file writer and worker.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_TEXT_FILE_WRITER_HPP
#define SMYD_TEXT_FILE_WRITER_HPP

#include "revision.hpp"
#include "worker.hpp"
#include <string>
#include <glib.h>
#include <gio/gio.h>

namespace Samoyed
{

class TextBuffer;

/**
 * A text file writer converts text into the external character encoding and
 * writes it to a file.
 */
class TextFileWriter
{
public:
    TextFileWriter();

    ~TextFileWriter();

    void open(const char *uri);

    void write(const char *text);

    void close();

    const Revision &revision() const
    { return m_revision; }

    GError *fetchError()
    {
        GError *error = m_error;
        m_error = NULL;
        return error;
    }

private:
    Revision m_revision;

    GError *m_error;
};

/**
 * Worker wrapping a text file writer.
 */
class TextFileWriteWorker: public Worker
{
public:
    TextFileWriteWorker(unsigned int priority,
                        const Callback &callback,
                        const char *uri,
                        const char *text):
        Worker(priority, callback),
        m_uri(uri),
        m_text(text)
    {}

    TextFileWriter &writer() { return m_writer; }

    virtual bool step();
    
private:
    std::string m_uri;

    std::string m_text;

    TextFileWriter m_writer;
};

}

#endif
