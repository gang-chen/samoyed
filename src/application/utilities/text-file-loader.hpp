// Text file reader and worker.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_TEXT_FILE_READER_HPP
#define SMYD_TEXT_FILE_READER_HPP

#include "revision.hpp"
#include "worker.hpp"
#include <string>
#include <glib.h>
#include <gio/gio.h>

namespace Samoyed
{

class TextBuffer;

/**
 * A text file reader reads a text file, converts its contents into UTF-8
 * encoded text and stored in a text buffer.
 */
class TextFileReader
{
public:
    TextFileReader();

    ~TextFileReader();

    void open(const char *uri);

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
    GFile *m_file;

    GFileInputStream *m_fileStream;

    GCharsetConverter *m_encodingConverter;

    GInputStream *m_converterStream;

    GInputStream *m_stream;

    TextBuffer *m_buffer;

    Revision m_revision;

    GError *m_error;
};

/**
 * Worker wrapping a text file reader.
 */
class TextFileReadWorker: public Worker
{
public:
    TextFileReadWorker(unsigned int priority,
                       const Callback &callback,
                       const char *uri,
                       bool convertEncoding):
        Worker(priority, callback),
        m_uri(uri)
    {}

    TextFileReader &reader() { return m_reader; }

    virtual bool step();
    
private:
    std::string m_uri;

    TextFileReader m_reader;
};

}

#endif
