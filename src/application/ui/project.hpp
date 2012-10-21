// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_HPP
#define SMYD_PROJECT_HPP

#include "../utilities/string-comparator.hpp"
#include "../utilities/manager.hpp"
#include <map>
#include <string>

namespace Samoyed
{

class Editor;
class ProjectConfiguration;

/**
 * A project represents an opened Samoyed project, which is a collection of well
 * organized resources.  Each opened physical project has one and only one
 * project instance.
 *
 * Projects are user interface objects that can be accessed in the main thread
 * only.
 *
 * The contents of a project are a project configuration.  The project is the
 * only writer of the project configuration.
 */
class Project
{
public:
    Editor *findEditor(const char *uri);

    void addEditor(Editor &editor);

    void removeEditor(Editor &editor);

private:
    typedef std::map<const char *, Editor *, StringComparator> EditorTable;

    const std::string m_uri;

    const std::string m_name;

    ReferencePoint<ProjectConfiguration> m_config;

    /**
     * The editors in the context of this project.
     */
    EditorTable m_editors;
};

}

#endif
