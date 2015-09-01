// Plugin: GNU build system.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "gnu-build-system-plugin.hpp"
#include "gnu-build-system-extension.hpp"
#include <string.h>
#include <gmodule.h>

namespace Samoyed
{

namespace GnuBuildSystem
{

GnuBuildSystemPlugin *GnuBuildSystemPlugin::s_instance = NULL;

GnuBuildSystemPlugin::GnuBuildSystemPlugin(PluginManager &manager,
                                           const char *id,
                                           GModule *module):
    Plugin(manager, id, module)
{
    s_instance = this;
}

Extension *GnuBuildSystemPlugin::createExtension(const char *extensionId)
{
    if (strcmp(extensionId, "gnu-build-system/build-system") == 0)
        return new GnuBuildSystemExtension(extensionId, *this);
    return NULL;
}

void GnuBuildSystemPlugin::onBuildSystemCreated(GnuBuildSystem &buildSystem)
{
    m_buildSystems.insert(&buildSystem);
}

void GnuBuildSystemPlugin::onBuildSystemDestroyed(GnuBuildSystem &buildSystem)
{
    m_buildSystems.erase(&buildSystem);
    if (completed())
        onCompleted();
}

bool GnuBuildSystemPlugin::completed() const
{
    return m_buildSystems.empty();
}

void GnuBuildSystemPlugin::deactivate()
{
    // TBD: Close all GNU build system projects?
}

}

}

extern "C"
{

G_MODULE_EXPORT
Samoyed::Plugin *createPlugin(Samoyed::PluginManager *manager,
                              const char *id,
                              GModule *module,
                              std::string *error)
{
    return new Samoyed::GnuBuildSystem::GnuBuildSystemPlugin(*manager,
                                                             id,
                                                             module);
}

}
