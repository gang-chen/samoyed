// File editor.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "editor.hpp"
#include "file.hpp"
#include "project.hpp"
#include "widget-container.hpp"
#include "window.hpp"
#include "application.hpp"
#include <string.h>
#include <list>
#include <memory>
#include <string>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <libxml/tree.h>

#define WIDGET "widget"
#define EDITOR "editor"
#define FILE_URI "file-uri"
#define PROJECT_URI "project-uri"
#define FILE_MIME_TYPE "file-mime-type"
#define FILE_OPTIONS "file-options"

#define EDITOR_ID "editor"

namespace
{

int serialNumber = 0;

}

namespace Samoyed
{

bool Editor::XmlElement::readInternally(xmlNodePtr node,
                                        std::list<std::string> &errors)
{
    char *value, *cp;
    bool widgetSeen = false;
    bool fileUriSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
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
            if (!Widget::XmlElement::readInternally(child, errors))
                return false;
            widgetSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        FILE_URI) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_fileUri = value;
                xmlFree(value);
                fileUriSeen = true;
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        PROJECT_URI) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_projectUri = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        FILE_MIME_TYPE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_fileMimeType = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        m_fileOptions.name()) == 0)
            m_fileOptions.readXmlElement(child, errors);
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
    if (!fileUriSeen)
    {
        cp = g_strdup_printf(
            _("Line %d: \"%s\" element missing.\n"),
            node->line, FILE_URI);
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
                    reinterpret_cast<const xmlChar *>(FILE_URI),
                    reinterpret_cast<const xmlChar *>(fileUri()));
    if (projectUri())
        xmlNewTextChild(node, NULL,
                        reinterpret_cast<const xmlChar *>(PROJECT_URI),
                        reinterpret_cast<const xmlChar *>(projectUri()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(FILE_MIME_TYPE),
                    reinterpret_cast<const xmlChar *>(fileMimeType()));
    xmlAddChild(node, m_fileOptions.writeXmlElement());
    return node;
}

Editor::XmlElement::XmlElement(const Editor &editor):
    Widget::XmlElement(editor),
    m_fileUri(editor.file().uri()),
    m_fileMimeType(editor.file().mimeType()),
    m_fileOptions(*std::auto_ptr<PropertyTree>(editor.file().options()))
{
    if (editor.project())
        m_projectUri = editor.project()->uri();

    // Remove the edited mark from the title.
    std::string title = this->title();
    if (editor.file().edited())
        setTitle(title.c_str() + 2);
}

Editor *Editor::XmlElement::createEditor()
{
    Project *project = NULL;
    if (projectUri())
    {
        project = Application::instance().findProject(projectUri());
        if (!project)
            m_projectUri.clear();
    }
    Editor *editor =
        File::open(fileUri(), project, fileMimeType(), fileOptions(), true).
        second;
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
    std::string id(EDITOR_ID "-");
    id += boost::lexical_cast<std::string>(serialNumber++);
    if (!Widget::setup(id.c_str()))
        return false;
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
        try
        {
            int sn = boost::lexical_cast<int>(cp + 1);
            if (sn >= serialNumber)
                serialNumber = sn + 1;
        }
        catch (boost::bad_lexical_cast &)
        {
        }
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
    std::string title = this->title();
    if (m_file.edited())
    {
        title.insert(0, "* ");
        setTitle(title.c_str());
    }
    else
        setTitle(title.c_str() + 2);
}

void Editor::destroyInFile()
{
    if (m_project)
        m_project->destroyEditor(*this);
    else
        destroy();
}

void Editor::destroyInProject()
{
    destroy();
}

gboolean Editor::onFocusIn(GtkWidget *widget,
                           GdkEventFocus *event,
                           Editor *editor)
{
    Widget *window = editor;
    if (window->parent())
    {
        do
            window = window->parent();
        while (window->parent());
        static_cast<Window *>(window)->onCurrentFileChanged(
            editor->file().uri());
    }
    return FALSE;
}

}
