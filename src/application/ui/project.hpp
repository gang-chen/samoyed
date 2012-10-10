// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_HPP
#define SMYD_PROJECT_HPP

namespace Samoyed
{

class ProjectDevelopmentEnvironment;

/**
 * A project represents an opened Samoyed project, which is a collection of well
 * organized resources.  Each opened physical project has one and only one
 * project instance.  Each project has one and only one project development
 * environment, where the user may modify the project.
 *
 * Projects are user interface objects that can be accessed in the main thread
 * only.
 */
class Project
{
private:
    ProjectDevelopmentEnvironment *m_environment;
};

}

#endif
