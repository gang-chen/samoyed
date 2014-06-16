// Text file loader.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_TEXT_FILE_LOADER_HPP
#define SMYD_TEXT_FILE_LOADER_HPP

#include "file-loader.hpp"
#include <string>
#include <gio/gio.h>

namespace Samoyed
{

class TextBuffer;

/**
 * A text file loader reads a text file, converts its contents into UTF-8
 * encoded text and stored in a text buffer.
 */
class TextFileLoader: public FileLoader
{
public:
    TextFileLoader(Scheduler &scheduler,
                   unsigned int priority,
                   const Callback &callback,
                   const char *uri,
                   const char *encoding);

    virtual ~TextFileLoader();

    const TextBuffer *buffer() const { return m_buffer; }

    TextBuffer *takeBuffer()
    {
        TextBuffer *buffer = m_buffer;
        m_buffer = NULL;
        return buffer;
    }

    virtual bool step();

private:
    void cleanUp();

    std::string m_encoding;

    TextBuffer *m_buffer;

    GFile *m_file;
    GFileInputStream *m_fileStream;
    GCharsetConverter *m_encodingConverter;
    GInputStream *m_converterStream;
    GInputStream *m_stream;
    char *m_readBuffer;
    char *m_readPointer;
};

}

#endif
