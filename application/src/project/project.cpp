// Open project.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project.hpp"
#include "project-db.hpp"
#include "build-system/build-system.hpp"
#include "build-system/build-systems-extension-point.hpp"
#include "editors/editor.hpp"
#include "window/window.hpp"
#include "plugin/extension-point-manager.hpp"
#include "utilities/miscellaneous.hpp"
#include "application.hpp"
#include <assert.h>
#include <list>
#include <map>
#include <numeric>
#include <string>
#include <utility>
#include <glib.h>
#include <glib/gi18n.h>
#include <libxml/tree.h>
#include <sqlite3.h>

#define PROJECT "project"
#define BUILD_SYSTEM "build-system"
#define URI "uri"
#define ACTIVE_CONFIGURATION "active-configuration"
#define BUILD_SYSTEMS "build-systems"

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
    Project *project = Project::open(uri());
    if (!project->restore(*this))
    {
        delete project;
        return NULL;
    }
    return project;
}

Project::Project(const char *uri):
    m_uri(uri),
    m_db(NULL),
    m_buildSystem(NULL)
{
    Application::instance().addProject(*this);
}

Project::~Project()
{
    assert(!m_db);
    delete m_buildSystem;
    Application::instance().removeProject(*this);
}

Project *Project::create(const char *uri,
                         const char *buildSystemExtensionId)
{
}

bool Project::readProjectXmlFile(xmlNodePtr node,
                                 std::list<std::string> *errors)
{
    char *value, *cp;
    if (strcmp(reinterpret_cast<const char *>(node->name),
               PROJECT) != 0)
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: Root element is \"%s\"; should be \"%s\".\n"),
                node->line,
                reinterpret_cast<const char *>(node->name),
                PROJECT);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   BUILD_SYSTEM) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                setBuildSystem(value);
                xmlFree(value);
            }
        }
    }
    return true;
}

Project *Project::open(const char *uri)
{
    Project *project = Application::instance().findProject(uri);
    if (project)
        return project;

    project = new Project(uri);

    // Read the project file.
    std::string xmlUri(uri);
    xmlUri += G_DIR_SEPARATOR_S "samoyed-project.xml";
    GError *error = NULL;
    char *xmlFileName = g_filename_from_uri(xmlUri.c_str(), NULL, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open project \"%s\"."),
              uri);
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to parse URI \"%s\". %s."),
            uri, error->message);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        delete project;
        return NULL;
    }
    xmlDocPtr doc = xmlParseFile(xmlFileName);
    g_free(xmlFileName);
    if (!doc)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open project \"%s\"."),
              uri);
        xmlErrorPtr error = xmlGetLastError();
        if (error)
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to parse project file \"%s\". %s."),
                  xmlUri.c_str(), error->message);
        else
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to parse project file \"%s\"."),
                  xmlUri.c_str());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        delete project;
        return NULL;
    }
    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open project \"%s\"."),
              uri);
        gtkMessageDialogAddDetails(
            dialog,
            _("Project file \"%s\" is empty."),
            xmlUri.c_str());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        xmlFreeDoc(doc);
        delete project;
        return NULL;
    }
    std::list<std::string> errors;
    if (!project->readProjectXmlFile(node, &errors))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open project \"%s\"."),
              uri);
        if (errors.empty())
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed found errors in project file \"%s\"."),
                xmlUri.c_str());
        else
        {
            std::string e =
                std::accumulate(errors.begin(), errors.end(), std::string());
            gtkMessageDialogAddDetails(
                dialog,
                _("End errors in project file \"%s\":\n%s"),
                xmlUri.c_str(), e.c_str());
        }
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        xmlFreeDoc(doc);
        delete project;
        return NULL;
    }
    xmlFreeDoc(doc);

    // Open the project database.
    std::string dbUri(uri);
    dbUri += G_DIR_SEPARATOR_S ".samoyed" G_DIR_SEPARATOR_S "project.db";
    int dbError = ProjectDb::open(dbUri.c_str(), project->m_db);
    if (dbError != SQLITE_OK)
    {
        // Report the error.
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open project \"%s\"."),
              uri);
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to open project database \"%s\". %s."),
            dbUri.c_str(), sqlite3_errstr(dbError));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        delete project;
        return NULL;
    }

    return project;
}

bool Project::finishClosing()
{
    // Close the project database.
    int dbError = m_db->close();
    if (dbError != SQLITE_OK)
    {
        // Report the error.
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to close project \"%s\". Force to close it?"),
              uri());
        std::string dbUri(uri());
        dbUri += G_DIR_SEPARATOR_S ".samoyed" G_DIR_SEPARATOR_S "project.db";
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to close project database \"%s\". %s."),
            dbUri.c_str(), sqlite3_errstr(dbError));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response != GTK_RESPONSE_YES)
        {
            cancelClosing();
            return false;
        }
    }
    delete m_db;
    m_db = NULL;

    Application::instance().destroyProject(*this);
    return true;
}

bool Project::close()
{
    m_closing = true;
    if (!m_firstEditor)
        return finishClosing();

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
    setActiveConfiguration(xmlElement.activeConfiguration());
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
        finishClosing();
}

void Project::setBuildSystem(const char *buildSystemExtensionId)
{
    delete m_buildSystem;
    m_buildSystem =
        static_cast<BuildSystemsExtensionPoint &>(Application::instance().
        extensionPointManager().extensionPoint(BUILD_SYSTEMS)).
        activateBuildSystem(buildSystemExtensionId, *this);
}

}
