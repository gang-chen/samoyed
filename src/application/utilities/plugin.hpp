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
    static Plugin *activate(const char *id);

    bool deactivate();

    void addToCache(Plugin *lru, Plugin *mru);
    void removeFromCache(Plugin *lru, Plugin *mru);

protected:
    virtual bool startUp() { return true; }

    virtual void shutDown() {}

    virtual ~Plugin() {}

private:
    std::string m_id;

    Plugin *m_nextCached;
    Plugin *m_prevCached;
};

}

#endif
