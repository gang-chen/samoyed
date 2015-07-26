// Build system.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_SYSTEM_HPP
#define SMYD_BUILD_SYSTEM_HPP

namespace Samoyed
{

class BuildSystem
{
public:
    BuildSystem(Project &project): m_project(project) {}

    /**
     *
     */
    virtual bool addFile(const char *fileName) = 0;
    virtual bool removeFile(const char *fileName) = 0;
    virtual bool editFileProperties(const char *fileName) = 0;
    virtual bool importFile(const char *fileName) = 0;

    virtual const char *defaultConfigurationCommands() = 0;
    virtual const char *defaultBuildCommands() = 0;
    virtual const char *defaultInstallCommands() = 0;
    virtual const char *defaultDryBuildCommands() = 0;

    Project &project() { return m_project; }
    const Project &project() const { return m_project; }

private:
    Project &m_project;
};

}

#endif
