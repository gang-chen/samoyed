// Text file saver.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_TEXT_FILE_SAVER_HPP
#define SMYD_TEXT_FILE_SAVER_HPP

#include "file-saver.hpp"
#include <string>

namespace Samoyed
{

/**
 * A text file saver converts text into the external character encoding and
 * writes it to a file.
 */
class TextFileSaver: public FileSaver
{
public:
    TextFileSaver(unsigned int priority,
                  const Callback &callback,
                  const char *uri,
                  const char *text):
        FileSaver(priority, callback, uri),
        m_text(text)
    {}

    virtual bool step();
    
private:
    std::string m_text;
};

}

#endif
