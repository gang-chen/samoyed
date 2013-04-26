// Extension.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_EXTENSION_HPP
#define SMYD_EXTENSION_HPP

#include <string>

namespace Samoyed
{

class Plugin;

class Extension: public boost::noncopyable
{
private:
    std::string m_id;

    const Plugin &m_plugin;
};

}

#endif
