// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project.hpp"
#include "project-db.hpp"
#include "editors/editor.hpp"
#include "utilities/miscellaneous.hpp"
#include "application.hpp"
#include <assert.h>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <glib.h>
#include <glib/gi18n.h>
#include <libxml/tree.h>
#include <sqlite3.h>

#define PROJECT "project"
#define URI "uri"
#define ACTIVE_CONFIGURATION "active-configuration"

namespace Samoyed
{

bool Project::XmlElement::readInternally(xmlNodePtr node,
                                         std::list<std::string> *errors)
{
    char *value, *cp;
    bool uriSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   URI) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_uri = value;
                xmlFree(value);
                uriSeen = true;
            }
        }
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   ACTIVE_CONFIGURATION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_activeConfig = value;
                xmlFree(value);
            }
        }
    }

    if (!uriSeen)
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: \"%s\" element missing.\n"),
                node->line, URI);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }
    return true;
}

Project::XmlElement *Project::XmlElement::read(xmlNodePtr node,
                                               std::list<std::string> *errors)
{
    XmlElement *element = new XmlElement;
    if (!element->readInternally(node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr Project::XmlElement::write() const
{
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(PROJECT));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(URI),
                    reinterpret_cast<const xmlChar *>(uri()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(ACTIVE_CONFIGURATION),
                    reinterpret_cast<const xmlChar *>(activeConfiguration()));
    return node;
}

Project::XmlElement::XmlElement(const Project &project):
    m_uri(project.uri()),
    m_activeConfig(project.activeConfiguration())
{
}

Project *Project::XmlElement::restoreProject()
{
    if (Application::instance().findProject(uri()))
        return NULL;
    Project *project = new Project(uri());
    if (!project->restore(*this))
    {
        delete project;
        return NULL;
    }
    return project;
}

bool Project::openDb()
{
    std::string dbUri(m_uri);
    dbUri += G_DIR_SEPARATOR_S ".samoyed" G_DIR_SEPARATOR_S "project.db";
    int error = ProjectDb::open(dbUri.c_str(), m_db);
    if (error == SQLITE_OK)
        return true;

    // Report the error.
    return false;
}

bool Project::closeDb()
{
    int error = m_db->close();
    if (error == SQLITE_OK)
    {
        m_db = NULL;
        return true;
    }

    // Report the error.
    return false;
}

Project::Project(const char *uri):
    m_uri(uri),
    m_db(NULL)
{
    Application::instance().addProject(*this);
}

Project::~Project()
{
    assert(!m_db);
    Application::instance().removeProject(*this);
}

Project *Project::open(const char *uri)
{
    Project *project = Application::instance().findProject(uri);
    if (project)
        return project;
    project = new Project(uri);
    if (!project->openDb())
    {
        delete project;
        return NULL;
    }
    return project;
}

bool Project::close()
{
    m_closing = true;
    if (!m_firstEditor)
    {
        // Close the database.
        if (!closeDb())
            return false;
        Application::instance().destroyProject(*this);
        return true;
    }

    // Close all editors.
    for (Editor *editor = m_firstEditor, *next; editor; editor = next)
    {
        next = editor->nextInProject();
        if (!editor->close())
        {
            m_closing = false;
            return false;
        }
    }
    return true;
}

void Project::cancelClosing()
{
    m_closing = false;
    if (Application::instance().quitting())
        Application::instance().cancelQuitting();
}

Project::XmlElement *Project::save() const
{
    return new XmlElement(*this);
}

bool Project::restore(XmlElement &xmlElement)
{
    m_activeConfig = xmlElement.activeConfiguration();
    return true;
}

Editor *Project::findEditor(const char *uri)
{
    EditorTable::const_iterator it = m_editorTable.find(uri);
    if (it == m_editorTable.end())
        return NULL;
    return it->second;
}

const Editor *Project::findEditor(const char *uri) const
{
    EditorTable::const_iterator it = m_editorTable.find(uri);
    if (it == m_editorTable.end())
        return NULL;
    return it->second;
}

void Project::addEditor(Editor &editor)
{
    m_editorTable.insert(std::make_pair(editor.file().uri(), &editor));
    editor.addToListInProject(m_firstEditor, m_lastEditor);
}

void Project::removeEditor(Editor &editor)
{
    std::pair<EditorTable::iterator, EditorTable::iterator> range =
        m_editorTable.equal_range(editor.file().uri());
    for (EditorTable::iterator it = range.first; it != range.second; ++it)
        if (it->second == &editor)
        {
            m_editorTable.erase(it);
            break;
        }
    editor.removeFromListInProject(m_firstEditor, m_lastEditor);
}

void Project::destroyEditor(Editor &editor)
{
    editor.destroyInProject();
    if (m_closing && !m_firstEditor)
    {
        if (!closeDb())
            cancelClosing();
        else
            Application::instance().destroyProject(*this);
    }
}

}
