// Text file writer and worker.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_TEXT_FILE_WRITER_HPP
#define SMYD_TEXT_FILE_WRITER_HPP

#include "revision.hpp"
#include "worker.hpp"
#include <glib.h>

namespace Samoyed
{

class TextBuffer;

/**
 */
class TextFileWriter
{
public:
    TextFileWriter();

    ~TextFileWriter();

    void open(const char *uri, bool convertEncoding);

    void read();

    void close();

    TextBuffer *fetchBuffer()
    {
        TextBuffer *buffer = m_buffer;
        m_buffer = NULL;
        return buffer;
    }

    const Revision &revision() const
    { return m_revision; }

    GError *fetchError()
    {
        GError *error = m_error;
        m_error = NULL;
        return error;
    }

private:
    TextBuffer *m_buffer;

    Revision m_revision;

    GError *m_error;
};

/**
 * Worker wrapping a text file reader.
 */
class TextFileWriterWorker: public Worker
{
public:
    TextFiledWorker(unsigned int priority,
                       const Callback &callback,
                       const char *uri,
                       bool convertEncoding):
        Worker(priority, callback),
        m_uri(uri),
        m_convertEncoding(convertEncoding)
    {}
    TextFileReader &reader() { return m_reader; }
    virtual bool step();
    
private:
    std::string m_uri;
    bool m_convertEncoding;
    TextFileReader m_reader;
};

}

#endif
