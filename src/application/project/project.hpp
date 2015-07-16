// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_HPP
#define SMYD_PROJECT_HPP

#include "utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class ProjectDb;
class Editor;

/**
 * A project represents an open Samoyed project, which is a collection of well
 * organized resources.  Each open physical project has one and only one
 * project instance.
 *
 * Projects are user interface objects that can be accessed in the main thread
 * only.
 */
class Project: public boost::noncopyable
{
public:
    class XmlElement
    {
    public:
        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        xmlNodePtr write() const;
        XmlElement(const Project &project);
        Project *restoreProject();

        const char *uri() const { return m_uri.c_str(); }
        const char *activeConfiguration() const
        { return m_activeConfig.c_str(); }

    private:
        XmlElement() {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);

        std::string m_uri;
        std::string m_activeConfig;
    };

    static Project *create(const char *uri);

    static Project *open(const char *uri);

    static void remove(const char *uri);

    static Project *createByDialog(const char *uri);

    static Project *openByDialog(const char *uri);

    static void removeByDialog(const char *uri);

    /**
     * This function can be called by the application instance only.
     */
    ~Project();

    bool closing() const { return m_closing; }

    bool close();

    void cancelClosing();

    XmlElement *save() const;

    const char *uri() const { return m_uri.c_str(); }

    const char *activeConfiguration() const { return m_activeConfig.c_str(); }
    void setActiveConfiguration(const char *config)
    { m_activeConfig = config; }

    ProjectDb &db() { return *m_db; }

    Editor *findEditor(const char *uri);
    const Editor *findEditor(const char *uri) const;

    void addEditor(Editor &editor);
    void removeEditor(Editor &editor);

    /**
     * This function is called by the editor after the file processed the editor
     * close request.
     */
    void destroyEditor(Editor &editor);

    Editor *editors() { return m_firstEditor; }
    const Editor *editors() const { return m_firstEditor; }

protected:
    Project(const char *uri);

    bool restore(XmlElement &xmlElement);

private:
    typedef std::multimap<ComparablePointer<const char>, Editor *>
        EditorTable;

    bool openDb();
    bool closeDb();

    const std::string m_uri;

    std::string m_activeConfig;

    ProjectDb *m_db;

    bool m_closing;

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
