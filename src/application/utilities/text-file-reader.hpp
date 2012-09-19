// Text file reader.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_TEXT_FILE_READER_HPP
#define SMYD_TEXT_FILE_READER_HPP

#include "revision.hpp"
#include <glib.h>

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
    TextBuffer *m_buffer;

    Revision m_revision;

    GError *m_error;
};

}

#endif
