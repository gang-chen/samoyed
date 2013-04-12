// File editor.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "editor.hpp"
#include "file.hpp"
#include "project.hpp"
#include "../application.hpp"
#include <stdlib.h>
#include <string.h>
#include <list>
#include <string>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/tree.h>

#define WIDGET "widget"
#define EDITOR "editor"
#define URI "uri"
#define PROJECT_URI "project-uri"

#define EDITOR_ID "editor"

namespace
{

int serialNumber = 0;

}

namespace Samoyed
{

bool Editor::XmlElement::readInternally(xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors)
{
    char *value, *cp;
    bool widgetSeen = false;
    bool uriSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   WIDGET) == 0)
        {
            if (widgetSeen)
            {
                cp = g_strdup_printf(
                    _("Line %d: More than one \"%s\" elements seen.\n"),
                    child->line, WIDGET);
                errors.push_back(cp);
                g_free(cp);
                return false;
            }
            if (!Widget::XmlElement::readInternally(doc, child, errors))
                return false;
            widgetSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        URI) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_uri = value;
            xmlFree(value);
            uriSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        PROJECT_URI) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_projectUri = value;
            xmlFree(value);
        }
    }

    if (!widgetSeen)
    {
        cp = g_strdup_printf(
            _("Line %d: \"%s\" element missing.\n"),
            node->line, WIDGET);
        errors.push_back(cp);
        g_free(cp);
        return false;
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

xmlNodePtr Editor::XmlElement::write() const
{
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(EDITOR));
    xmlAddChild(node, Widget::XmlElement::write());
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(URI),
                    reinterpret_cast<const xmlChar *>(uri()));
    if (projectUri())
        xmlNewTextChild(node, NULL,
                        reinterpret_cast<const xmlChar *>(PROJECT_URI),
                        reinterpret_cast<const xmlChar *>(projectUri()));
    return node;
}

Editor::XmlElement::XmlElement(const Editor &editor)
{
    m_uri = editor.file().uri();
    if (editor.project())
        m_projectUri = editor.project()->uri();
}

Editor *Editor::XmlElement::restoreEditor(
    std::map<std::string, boost::any> &options)
{
    Project *project = NULL;
    if (projectUri())
    {
        project = Application::instance().findProject(projectUri());
        if (!project)
            m_projectUri.clear();
    }
    Editor *editor = File::open(uri(), project, options, true).second;
    return editor;
}

Editor::Editor(File &file, Project *project):
    m_file(file),
    m_project(project)
{
    if (m_project)
        m_project->addEditor(*this);
    m_file.addEditor(*this);
}

Editor::~Editor()
{
    m_file.removeEditor(*this);
    if (m_project)
        m_project->removeEditor(*this);
}

bool Editor::setup()
{
    char *id = g_strdup_printf(EDITOR_ID "-%d", serialNumber++);
    if (!Widget::setup(id))
    {
        g_free(id);
        return false;
    }
    g_free(id);
    char *fileName = g_filename_from_uri(m_file.uri(), NULL, NULL);
    char *title = g_filename_display_basename(fileName);
    setTitle(title);
    g_free(title);
    g_free(fileName);
    setDescription(m_file.uri());
    return true;
}

bool Editor::restore(XmlElement &xmlElement)
{
    if (!Widget::restore(xmlElement))
        return false;
    // Extract the serial number of the editor from its identifier, and update
    // the global serial number.
    const char *cp = strrchr(id(), '-');
    if (cp)
    {
        int sn = atoi(cp + 1);
        if (sn >= serialNumber)
            serialNumber = sn + 1;
    }
    return true;
}

bool Editor::close()
{
    if (closing())
        return true;

    setClosing(true);
    if (!m_file.closeEditor(*this))
    {
        setClosing(false);
        return false;
    }
    return true;
}

void Editor::onFileEditedStateChanged()
{
    if (m_file.edited())
    {
        std::string title(this->title());
        title.insert(0, "* ");
        setTitle(title.c_str());
    }
    else
        setTitle(title() + 2);
}

}
