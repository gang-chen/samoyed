// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_HPP
#define SMYD_PROJECT_HPP

#include "../utilities/manager.hpp"

namespace Samoyed
{

class ProjectConfiguration;
class ProjectDevelopmentEnvironment;

/**
 * A project represents an opened Samoyed project, which is a collection of well
 * organized resources.  Each opened physical project has one and only one
 * project instance.
 *
 * Projects are user interface objects that can be accessed in the main thread
 * only.
 *
 * Projects are managed by the project manager.
 *
 * The contents of a project are a project configuration.  The project is the
 * only writer of the project configuration.
 * 
 * A project has a project development environment, where the user can edit the
 * project.
 */
class Project
{
private:
    ReferencePoint<ProjectConfiguration> m_config;

    ProjectDevelopmentEnvironment *m_environment;
};

}

#endif
