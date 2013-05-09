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
 * A plugin adds functionality to the host application or host plugins.  A
 * plugin contributes its functionality by plugging some extensions into the
 * extension points defined by the host application or host plugins.  A plugin
 * will be automatically activated by the plugin manager when any extension it
 * provides is used.  A plugin will be automatically deactivated by the plugin
 * manager when none of its extension is used and its task is completed.
 */
class Plugin: boost::noncopyable
{
public:
    static Plugin *activate(const char *id);

    bool forceDeactivate();

    void deactivate();

    Extension *referenceExtension(const char *extensionId);

    void onExtensionReleased(Extension &extension);

    bool cached() const { return m_nextCached; }

    void addToCache(Plugin *&lru, Plugin *&mru);
    void removeFromCache(Plugin *&lru, Plugin *&mru);

protected:
    /**
     * @return False if failed to start up the plugin, which will abort the
     * plugin activation.
     */
    virtual bool startUp() = 0;

    virtual void shutDown() = 0;

    Plugin(const char *id);

    virtual ~Plugin();

    void addExtension(Extension &extension);
    void removeExtension(Extension &extension);

private:
    typedef std::map<ComparablePointer<const char *>, Extension *>
        ExtensionTable;

    const std::string m_id;

    ExtensionTable m_extensionTable;

    Plugin *m_nextCached;
    Plugin *m_prevCached;
};

}

#endif
