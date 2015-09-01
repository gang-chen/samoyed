// Plugin: GNU build system.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_GNU_BUILD_SYSTEM_PLUGIN_HPP
#define SMYD_GNU_BUILD_SYSTEM_PLUGIN_HPP

#include "plugin/plugin.hpp"
#include <set>

namespace Samoyed
{

namespace GnuBuildSystem
{

class GnuBuildSystem;

class GnuBuildSystemPlugin: public Plugin
{
public:
    static GnuBuildSystemPlugin &instance() { return *s_instance; }

    GnuBuildSystemPlugin(PluginManager &manager,
                         const char *id,
                         GModule *module);

    virtual void deactivate();

    void onBuildSystemCreated(GnuBuildSystem &buildSystem);
    void onBuildSystemDestroyed(GnuBuildSystem &buildSystem);

protected:
    virtual Extension *createExtension(const char *extensionId);

    virtual bool completed() const;

private:
    static GnuBuildSystemPlugin *s_instance;

    std::set<GnuBuildSystem *> m_buildSystems;
};

}

}

#endif
