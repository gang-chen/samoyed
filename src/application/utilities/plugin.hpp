// Plugin.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PLUGIN_HPP
#define SMYD_PLUGIN_HPP

#include "miscellaneous.hpp"
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <gmodule.h>

namespace Samoyed
{

class PluginManager;
class Extension;

/**
 * A plugin adds functionality to the host application or plugins.  A plugin
 * contributes its functionality by plugging some extensions into the extension
 * points defined by the host application or plugins.  A plugin will be
 * activated by the plugin manager when any extension it provides is requested.
 * A plugin will be automatically deactivated by the plugin manager when none of
 * its extension is used and its tasks are completed.
 */
class Plugin: public boost::noncopyable
{
public:
    static Plugin *activate(PluginManager &manager,
                            const char *id,
                            const char *moduleFileName,
                            std::string &error);

    const char *id() const { return m_id.c_str(); }

    /**
     * Request the extension points to release the extensions and stop the
     * running tasks so that the plugin can be deactivated.
     * @return True iff the plugin is deactivated.
     */
    bool deactivate();

    Extension *acquireExtension(const char *extensionId);

    void releaseExtension(Extension &extension);

    void destroy();

    bool cached() const { return m_nextCached; }

    void addToCache(Plugin *&lru, Plugin *&mru);
    void removeFromCache(Plugin *&lru, Plugin *&mru);

protected:
    Plugin(PluginManager &manager, const char *id, GModule *module);

    virtual ~Plugin();

    virtual Extension *createExtension(const char *extensionId) = 0;

    virtual bool completed() const = 0;

    void onCompleted();

private:
    typedef std::map<ComparablePointer<const char>, Extension *>
        ExtensionTable;

    PluginManager &m_manager;

    const std::string m_id;

    GModule *m_module;

    ExtensionTable m_extensionTable;

    int m_nActiveExtensions;

    Plugin *m_nextCached;
    Plugin *m_prevCached;
};

}

#endif
