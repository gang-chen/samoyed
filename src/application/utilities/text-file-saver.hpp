// Text file saver.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_TEXT_FILE_SAVER_HPP
#define SMYD_TEXT_FILE_SAVER_HPP

#include "file-saver.hpp"
#include <string>
#include <glib.h>

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
                  char *text,
                  int length,
                  const char *encoding);

    virtual ~TextFileSaver()
    { g_free(m_text); }

    virtual bool step();

private:
    char *m_text;

    int m_length;

    std::string m_encoding;
};

}

#endif
