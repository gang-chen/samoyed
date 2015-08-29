// Open project.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_HPP
#define SMYD_PROJECT_HPP

#include "utilities/miscellaneous.hpp"
#include <list>
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <boost/signals2/signal.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class ProjectDb;
class BuildSystem;
class ProjectFile;
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
    typedef boost::signals2::signal<void (Project &project)> Opened;
    typedef boost::signals2::signal<void (Project &project)> Close;

    class XmlElement
    {
    public:
        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        xmlNodePtr write() const;
        XmlElement(const Project &project);
        Project *restoreProject();

        const char *uri() const { return m_uri.c_str(); }

    private:
        XmlElement() {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);

        std::string m_uri;
    };

    static Project *create(const char *uri,
                           const char *buildSystemExtensionId);

    static Project *open(const char *uri);

    static void remove(const char *uri);

    static Project *openByDialog();

    static void removeByDialog();

    /**
     * This function can be called by the application instance only.
     */
    ~Project();

    bool closing() const { return m_closing; }

    bool close();

    void cancelClosing();

    XmlElement *save() const;

    const char *uri() const { return m_uri.c_str(); }

    ProjectDb &db() { return *m_db; }

    BuildSystem &buildSystem() { return *m_buildSystem; }
    const BuildSystem &buildSystem() const { return *m_buildSystem; }

    virtual ProjectFile *createProjectFile(int type) const;

    virtual bool addProjectFile(const char *uri, const ProjectFile &data);

    virtual bool removeProjectFile(const char *uri);

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

    static boost::signals2::connection
    addOpenedCallback(const Opened::slot_type &callback)
    { return s_opened.connect(callback); }

    boost::signals2::connection
    addCloseCallback(const Close::slot_type &callback)
    { return m_close.connect(callback); }

protected:
    Project(const char *uri);

    bool restore(XmlElement &xmlElement);

private:
    typedef std::multimap<ComparablePointer<const char>, Editor *>
        EditorTable;

    bool finishClosing();

    void setBuildSystem(const char *buildSystemExtensionId);

    bool readManifestFile(xmlNodePtr node, std::list<std::string> *errors);

    xmlNodePtr writeManifestFile() const;

    const std::string m_uri;

    ProjectDb *m_db;

    BuildSystem *m_buildSystem;

    bool m_closing;

    /**
     * The editors in the context of this project.
     */
    Editor *m_firstEditor;
    Editor *m_lastEditor;
    EditorTable m_editorTable;

    static Opened s_opened;
    Close m_close;

    SAMOYED_DEFINE_DOUBLY_LINKED(Project)
};

}

#endif
