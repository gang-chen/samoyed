// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project.hpp"
#include "editor.hpp"
#include "../application.hpp"
#include "../resources/project-configuration.hpp"
#include <list>
#include <map>
#include <string>
#include <utility>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/tree.h>

#define PROJECT "project"
#define URI "uri"

namespace Samoyed
{

bool Project::XmlElement::readInternally(xmlDocPtr doc,
                                         xmlNodePtr node,
                                         std::list<std::string> &errors)
{
    char *value, *cp;
    bool uriSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   URI) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_uri = value;
            xmlFree(value);
            uriSeen = true;
        }
    }

    if (!uriSeen)
    {
        cp = g_strdup_printf(
            _("Line %d: \"%s\" element missing.\n"),
            node->line, URI);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    return true;
}

Project::XmlElement *Project::XmlElement::read(xmlDocPtr doc,
                                               xmlNodePtr node,
                                               std::list<std::string> &errors)
{
    XmlElement *element = new XmlElement;
    if (!element->readInternally(doc, node, errors))
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
    return node;
}

Project::XmlElement::XmlElement(const Project &project):
    m_uri(project.uri())
{
}

Project *Project::XmlElement::restoreProject()
{
    Project *project = new Project(uri());
    if (!project->restore(*this))
    {
        delete project;
        return NULL;
    }
    return project;
}

Project::Project(const char *uri):
    m_uri(uri)
{
    Application::instance().addProject(*this);
}

Project::~Project()
{
    Application::instance().removeProject(*this);
}

Project *Project::create(const char *uri)
{
    return new Project(uri);
}

bool Project::close()
{
    m_closing = true;
    if (!m_firstEditor)
    {
        delete this;
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

Project::XmlElement *Project::save() const
{
    return new XmlElement(*this);
}

bool Project::restore(XmlElement &xmlElement)
{
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
    if (m_closing && !m_firstEditor)
        delete this;
}

}
