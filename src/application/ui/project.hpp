// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_HPP
#define SMYD_PROJECT_HPP

#include "../utilities/misc.hpp"
#include "../utilities/string-comparator.hpp"
#include "../utilities/manager.hpp"
#include <map>
#include <string>
#include <boost/noncopyable>

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
class Project: public boost::noncopyable
{
public:
    Project(const char *uri);

    ~Project();

    bool close();

    Editor *findEditor(const char *uri);
    const Editor *findEditor(const char *uri) const;

    void addEditor(Editor &editor);

    void removeEditor(Editor &editor);

    Editor *editors() { return m_firstEditor; }
    const Editor *editors() const { return m_firstEditor; }

private:
    typedef std::multi_map<ComparablePointer<const char *>, Editor *>
        EditorTable;

    const std::string m_uri;

    const std::string m_name;

    bool m_closing;

    ReferencePoint<ProjectConfiguration> m_config;

    /**
     * The editors in the context of this project.
     */
    Editor *m_firstEditor;
    Editor *m_lastEditor;
    EditorTable m_editorTable;

    SMYD_DEFINE_DOUBLY_LINKED(Project)
};

}

#endif
