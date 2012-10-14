// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_HPP
#define SMYD_PROJECT_HPP

#include "../utilities/manager.hpp"
#include "../utilities/pointer-comparator.hpp"
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
 * Projects are managed by the project manager.
 *
 * The contents of a project are a project configuration.  The project is the
 * only writer of the project configuration.
 */
class Project
{
public:
    Editor *getEditor(const char *uri);
    Editor &createEditor(const char *uri);
    bool destroyEditor(Editor &editor);

private:
    typedef std::map<std::string *, Editor *, PointerComparator<std::string> >
	    EditorTable;

    ReferencePoint<ProjectConfiguration> m_config;

    /**
     * The editors editing files in this project.
     */
    Editor *m_editors;

    EditorTable m_editorTable;
};

}

#endif
