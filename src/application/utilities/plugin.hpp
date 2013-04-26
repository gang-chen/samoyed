// Plugin.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PLUGIN_HPP
#define SMYD_PLUGIN_HPP

#include "miscellaneous.hpp"
#include <map>
#include <string>
#include <boost/utility.hpp>

namespace Samoyed
{

class Extension;

class Plugin: boost::noncopyable
{
public:
    static Plugin *activate(const char *id);

    bool deactivate();

    Extension *extension(const char *extensionId);

    bool cached() const { return m_nextCached; }

    void addToCache(Plugin *lru, Plugin *mru);
    void removeFromCache(Plugin *lru, Plugin *mru);

protected:
    /**
     * @return False if failed to start up the plugin, which will abort the
     * plugin activation.
     */
    virtual bool startUp() { return true; }

    /**
     * @return False if failed to shut down the plugin, which will abort the
     * plugin deactivation.
     */
    virtual bool shutDown() { return true; }

    virtual ~Plugin();

private:
    typedef std::map<ComparablePointer<const char *>, Extension *>
        ExtensionTable;

    std::string m_id;

    ExtensionTable m_extensionTable;

    Plugin *m_nextCached;
    Plugin *m_prevCached;
};

}

#endif
