// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_HPP
#define SMYD_PROJECT_HPP

#include "../utilities/miscellaneous.hpp"
#include "../utilities/manager.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <libxml/tree.h>

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
    class XmlElement
    {
    public:
        static XmlElement *read(xmlDocPtr doc,
                                xmlNodePtr node,
                                std::list<std::string> &errors);
        xmlNodePtr write() const;
        static XmlElement *saveProject();
        Project *restoreProject();

    private:
        XmlElement() {}

        std::string m_uri;
    };

    Project(const char *uri);

    bool close();

    XmlElement *save() const;

    const char *uri() const { return m_uri.c_str(); }

    const char *name() const { return m_name.c_str(); }

    Editor *findEditor(const char *uri);
    const Editor *findEditor(const char *uri) const;

    void addEditor(Editor &editor);

    void removeEditor(Editor &editor);

    void onEditorClosed();

    Editor *editors() { return m_firstEditor; }
    const Editor *editors() const { return m_firstEditor; }

protected:
    ~Project();

private:
    typedef std::multimap<ComparablePointer<const char *>, Editor *>
        EditorTable;

    const std::string m_uri;

    const std::string m_name;

    bool m_closing;

    ReferencePointer<ProjectConfiguration> m_config;

    /**
     * The editors in the context of this project.
     */
    Editor *m_firstEditor;
    Editor *m_lastEditor;
    EditorTable m_editorTable;

    SAMOYED_DEFINE_DOUBLY_LINKED(Project)
};

}

#endif
