// Text file saver.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_TEXT_FILE_SAVER_HPP
#define SMYD_TEXT_FILE_SAVER_HPP

#include "file-saver.hpp"
#include <string>
#include <boost/shared_ptr.hpp>
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
    TextFileSaver(Scheduler &scheduler,
                  unsigned int priority,
                  const char *uri,
                  const boost::shared_ptr<char> &text,
                  int length,
                  const char *encoding);

protected:
    virtual bool step();

private:
    boost::shared_ptr<char> m_text;

    int m_length;

    std::string m_encoding;
};

}

#endif
