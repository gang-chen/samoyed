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

/**
 * A plugin contains some extensions.  A plugin can be dynamically activated by
 * the plugin manager when any extension it provides is required.  A plugin can
 * be dynamically deactivated by the plugin manager when none of its extension
 * is required and its task is completed.
 */
class Plugin: boost::noncopyable
{
public:
    static Plugin *activate(const char *id);

    bool deactivate();

    Extension *loadExtension(const char *extensionId);

    void onExtensionReleased(Extension &extension);

    bool cached() const { return m_nextCached; }

    void addToCache(Plugin *&lru, Plugin *&mru);
    void removeFromCache(Plugin *&lru, Plugin *&mru);

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

    Plugin(const char *id);

    virtual ~Plugin();

    void addExtension(Extension &extension);
    void removeExtension(Extension &extension);

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
