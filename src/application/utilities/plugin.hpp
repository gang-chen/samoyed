// Plugin.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_PLUGIN_HPP
#define SMYD_PLUGIN_HPP

#include <string>

namespace Samoyed
{

class Plugin
{
public:
    Plugin(const char *id): m_id(id), m_refCount(0) {}

    /**
     * Activate this plugin if it is inactive.  Otherwise, increase the
     * reference count.
     * @return True iff the plugin is active.
     */
    bool activate();

    /**
     * Decrease the reference count, and if the reference count reaches zero,
     * deactive and destroy this plugin.
     * @return True iff the plugin is deactived and destroyed.
     */
    bool deactivate();

protected:
    virtual ~Plugin() {}

    virtual bool activateInterally() { return true; }

    virtual void deactiveInternally() {}

private:
    std::string m_id;

    int m_refCount;
};

}

#endif
