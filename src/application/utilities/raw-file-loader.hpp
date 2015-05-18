// Raw file loader.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_RAW_FILE_LOADER_HPP
#define SMYD_RAW_FILE_LOADER_HPP

#include "file-loader.hpp"
#include <boost/shared_ptr.hpp>

namespace Samoyed
{

class RawFileLoader: public FileLoader
{
public:
    RawFileLoader(Scheduler &scheduler,
                  unsigned int priority,
                  const char *uri);

    boost::shared_ptr<char> contents() const { return m_contents; }
    int length() const { return m_length; }

protected:
    virtual bool step();

    boost::shared_ptr<char> m_contents;
    unsigned int m_length;
};

}

#endif
