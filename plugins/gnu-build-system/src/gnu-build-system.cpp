// GNU build system.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "gnu-build-system.hpp"
#include "gnu-build-system-plugin.hpp"
#include "gnu-build-system-file.hpp"
#include "build-system/build-system.hpp"
#include "build-system/configuration.hpp"
#include "project/project-file.hpp"

namespace Samoyed
{

namespace GnuBuildSystem
{

GnuBuildSystem::GnuBuildSystem(Project &project, const char *extensionId):
    BuildSystem(project, extensionId)
{
    GnuBuildSystemPlugin::instance().onBuildSystemCreated(*this);
}

GnuBuildSystem::~GnuBuildSystem()
{
    GnuBuildSystemPlugin::instance().onBuildSystemDestroyed(*this);
}

bool GnuBuildSystem::setup()
{
    return true;
}

bool GnuBuildSystem::importFile(const char *uri)
{
    return true;
}

BuildSystemFile *GnuBuildSystem::createFile(int type) const
{
    if (type == ProjectFile::TYPE_DIRECTORY)
        return new BuildSystemFile;
    return new GnuBuildSystemFile;
}

bool GnuBuildSystem::addFile(const char *uri, const BuildSystemFile &data)
{
    return true;
}

bool GnuBuildSystem::removeFile(const char *uri)
{
    return true;
}

Configuration GnuBuildSystem::defaultConfiguration() const
{
    Configuration config(BuildSystem::defaultConfiguration());
    config.setConfigureCommands("./configure");
    config.setBuildCommands("make");
    config.setInstallCommands("make install");
    config.setCleanCommands("make clean");
    config.setCCompiler("gcc");
    config.setCppCompiler("g++");
    return config;
}

}

}
