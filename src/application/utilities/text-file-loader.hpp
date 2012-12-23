// Text file loader.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_TEXT_FILE_LOADER_HPP
#define SMYD_TEXT_FILE_LOADER_HPP

#include "file-loader.hpp"
#include <string>

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
    TextFileLoader(unsigned int priority,
                   const Callback &callback,
                   const char *uri):
        FileLoader(priority, callback, uri),
        m_buffer(NULL)
    {}

    virtual ~TextFileLoader();

    TextBuffer *takeBuffer()
    {
        TextBuffer *buffer = m_buffer;
        m_buffer = NULL;
        return buffer;
    }

    virtual bool step();

    virtual char *description() const;

private:
    TextBuffer *m_buffer;
};

}

#endif
