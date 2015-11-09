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
    switch (type)
    {
    case ProjectFile::TYPE_DIRECTORY:
        return new BuildSystemFile;
    case ProjectFile::TYPE_SOURCE_FILE:
        return new BuildSystemFile;
    case ProjectFile::TYPE_HEADER_FILE:
        return new GnuBuildSystemFile;
    case ProjectFile::TYPE_GENERIC_FILE:
        return new GnuBuildSystemFile;
    case ProjectFile::TYPE_STATIC_LIBRARY:
        return new GnuBuildSystemFile;
    case ProjectFile::TYPE_SHARED_LIBRARY:
        return new GnuBuildSystemFile;
    case ProjectFile::TYPE_PROGRAM:
        return new GnuBuildSystemFile;
    case ProjectFile::TYPE_GENERIC_TARGET:
        return new GnuBuildSystemFile;
    }
    return NULL;
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
