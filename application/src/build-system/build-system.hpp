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

    bool addFile(const char *uri);
    bool removeFile(const char *uri);
    bool editFileProperties(const char *uri);
    bool importFile(const char *uri);

    bool configure();
    bool build();
    bool install();
    bool collectCompilationOptions();

    Project &project() { return m_project; }
    const Project &project() const { return m_project; }

    virtual const char *defaultConfigureCommands() const { return ""; }
    virtual const char *defaultBuildCommands() const { return ""; }
    virtual const char *defaultInstallCommands() const { return ""; }
    virtual const char *defaultDryBuildCommands() const { return ""; }

private:
    Project &m_project;
};

}

#endif
