// Plugin.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PLUGIN_HPP
#define SMYD_PLUGIN_HPP

#include <string>
#include <boost/utility.hpp>

namespace Samoyed
{

class Plugin: boost::noncopyable
{
public:
    Plugin(const char *id): m_id(id) {}

    virtual bool startUp() { return true; }

    virtual void shutDown() {}

protected:
    virtual ~Plugin() {}

private:
    std::string m_id;
};

}

#endif
