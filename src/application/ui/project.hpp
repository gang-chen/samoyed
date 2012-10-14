// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_HPP
#define SMYD_PROJECT_HPP

#include "../utilities/manager.hpp"

namespace Samoyed
{

class ProjectConfiguration;
class Editor;

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
 */
class Project
{
public:
    Editor &createEditor(const char *uri);
    bool destroyEditor(Editor &editor);

private:
    ReferencePoint<ProjectConfiguration> m_config;

    /**
     * The editors editing files in this project.
     */
    Editor *m_editors;
};

}

#endif
