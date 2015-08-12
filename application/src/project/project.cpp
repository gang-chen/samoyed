// Open project.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project.hpp"
#include "project-db.hpp"
#include "configuration.hpp"
#include "build-system/build-system.hpp"
#include "build-system/build-systems-extension-point.hpp"
#include "editors/editor.hpp"
#include "window/window.hpp"
#include "plugin/extension-point-manager.hpp"
#include "utilities/miscellaneous.hpp"
#include "application.hpp"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <list>
#include <map>
#include <numeric>
#include <string>
#include <utility>
#include <glib.h>
#include <glib/gi18n.h>
#include <libxml/tree.h>
#include <db.h>

#define PROJECT "project"
#define BUILD_SYSTEM "build-system"
#define CONFIGURATIONS "configurations"
#define CONFIGURATION "configuration"
#define ACTIVE_CONFIGURATION "active-configuration"
#define URI "uri"
#define BUILD_SYSTEMS "build-systems"

namespace
{

void checkProjectExists(GtkFileChooser *chooser, gpointer dialog)
{
    char *dir = gtk_file_chooser_get_filename(chooser);
    if (!dir || !g_file_test(dir, G_FILE_TEST_IS_DIR))
    {
        gtk_widget_set_sensitive(
            gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog),
                                               GTK_RESPONSE_OK),
            FALSE);
        g_free(dir);
        return;
    }
    std::string file(dir);
    file += G_DIR_SEPARATOR_S "samoyed-project.xml";
    if (!g_file_test(file.c_str(), G_FILE_TEST_EXISTS))
        gtk_widget_set_sensitive(
            gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog),
                                               GTK_RESPONSE_OK),
            FALSE);
    else
        gtk_widget_set_sensitive(
            gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog),
                                               GTK_RESPONSE_OK),
            TRUE);
    g_free(dir);
}

}

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
    return node;
}

Project::XmlElement::XmlElement(const Project &project):
    m_uri(project.uri())
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
    m_buildSystem(NULL),
    m_activeConfig(NULL),
    m_closing(false),
    m_firstEditor(NULL),
    m_lastEditor(NULL)
{
    Application::instance().addProject(*this);
}

Project::~Project()
{
    assert(!m_db);
    assert(!m_firstEditor);
    assert(!m_lastEditor);
    delete m_buildSystem;
    ConfigurationTable::iterator it;
    while ((it = m_configurations.begin()) != m_configurations.end())
    {
        Configuration *config = it->second;
        m_configurations.erase(it);
        delete config;
    }
    Application::instance().removeProject(*this);
}

Project *Project::create(const char *uri,
                         const char *buildSystemExtensionId)
{
    // Check to see if the project already exists.
    std::string xmlUri(uri);
    xmlUri += "/samoyed-project.xml";
    GError *error = NULL;
    char *xmlFileName = g_filename_from_uri(xmlUri.c_str(), NULL, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create project \"%s\"."),
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
        return NULL;
    }
    if (g_file_test(xmlFileName, G_FILE_TEST_EXISTS))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create project \"%s\" because the project "
              "already exists."),
            uri);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(xmlFileName);
        return NULL;
    }

    Project *project = new Project(uri);
    project->setBuildSystem(buildSystemExtensionId);

    // Write the project manifest file.
    xmlNodePtr node = project->writeManifestFile();
    xmlDocPtr doc = xmlNewDoc(reinterpret_cast<const xmlChar *>("1.0"));
    xmlDocSetRootElement(doc, node);
    if (xmlSaveFormatFile(xmlFileName, doc, 1) == -1)
    {
        xmlErrorPtr error = xmlGetLastError();
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create project \"%s\"."),
            uri);
        if (error)
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to write project manifest file \"%s\". %s."),
                xmlUri.c_str(), error->message);
        else
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to write project manifest file \"%s\"."),
                xmlUri.c_str());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        xmlFreeDoc(doc);
        g_free(xmlFileName);
        delete project;
        return NULL;
    }
    xmlFreeDoc(doc);
    g_free(xmlFileName);

    // Create the project database.
    std::string dbUri(uri);
    dbUri += "/.samoyed/project-db";
    char *dbFileName = g_filename_from_uri(dbUri.c_str(), NULL, &error);
    if (g_mkdir_with_parents(dbFileName, 0775))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create project \"%s\"."),
            uri);
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to create directory \"%s\" to store the project "
              "database. %s."),
            dbUri.c_str(), g_strerror(errno));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free(dbFileName);
        delete project;
        return NULL;
    }
    g_free(dbFileName);
    int dbError = ProjectDb::create(dbUri.c_str(), project->m_db);
    if (dbError)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create project \"%s\"."),
            uri);
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to create project database \"%s\". %s."),
            dbUri.c_str(), db_strerror(dbError));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        delete project;
        return NULL;
    }

    return project;
}

bool Project::readManifestFile(xmlNodePtr node,
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
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CONFIGURATIONS) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                if (grandChild->type != XML_ELEMENT_NODE)
                    continue;
                if (strcmp(reinterpret_cast<const char *>(grandChild->name),
                           CONFIGURATION) == 0)
                {
                    Configuration *config = new Configuration;
                    if (config->readXmlElement(grandChild, errors))
                    {
                        if (!m_configurations.insert(
                                std::make_pair(config->name(), config)).second)
                            delete config;
                    }
                    else
                        delete config;
                }
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ACTIVE_CONFIGURATION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ConfigurationTable::iterator it = m_configurations.find(value);
                if (it != m_configurations.end())
                    m_activeConfig = it->second;
                xmlFree(value);
            }
        }
    }
    return true;
}

xmlNodePtr Project::writeManifestFile() const
{
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(PROJECT));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(BUILD_SYSTEM),
                    reinterpret_cast<const xmlChar *>(m_buildSystemExtensionId.
                                                      c_str()));
    xmlNodePtr configs =
        xmlNewNode(NULL,
                   reinterpret_cast<const xmlChar *>(CONFIGURATIONS));
    for (ConfigurationTable::const_iterator it = m_configurations.begin();
         it != m_configurations.end();
         ++it)
        xmlAddChild(configs, it->second->writeXmlElement());
    xmlAddChild(node, configs);
    if (m_activeConfig)
        xmlNewTextChild(node, NULL,
                        reinterpret_cast<const xmlChar *>(ACTIVE_CONFIGURATION),
                        reinterpret_cast<const xmlChar *>(m_activeConfig->
                                                          name()));
    return node;
}

Project *Project::open(const char *uri)
{
    Project *project = Application::instance().findProject(uri);
    if (project)
        return project;

    project = new Project(uri);

    // Read the project manifest file.
    std::string xmlUri(uri);
    xmlUri += "/samoyed-project.xml";
    GError *error = NULL;
    char *xmlFileName = g_filename_from_uri(xmlUri.c_str(), NULL, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
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
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open project \"%s\"."),
            uri);
        xmlErrorPtr error = xmlGetLastError();
        if (error)
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to parse project manifest file \"%s\". %s."),
                  xmlUri.c_str(), error->message);
        else
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to parse project manifest file \"%s\"."),
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
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open project \"%s\"."),
            uri);
        gtkMessageDialogAddDetails(
            dialog,
            _("Project manifest file \"%s\" is empty."),
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
    if (!project->readManifestFile(node, &errors))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open project \"%s\"."),
            uri);
        if (errors.empty())
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed found errors in project manifest file \"%s\"."),
                xmlUri.c_str());
        else
        {
            std::string e =
                std::accumulate(errors.begin(), errors.end(), std::string());
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed found errors in project manifest file \"%s\":\n%s"),
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
    dbUri += "/.samoyed/project-db";
    int dbError = ProjectDb::open(dbUri.c_str(), project->m_db);
    if (dbError)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to open project \"%s\"."),
            uri);
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to open project database \"%s\". %s."),
            dbUri.c_str(), db_strerror(dbError));
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
    // Write the project manifest file.
    std::string xmlUri(uri());
    xmlUri += "/samoyed-project.xml";
    GError *error = NULL;
    char *xmlFileName = g_filename_from_uri(xmlUri.c_str(), NULL, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to close project \"%s\". Force to close it?"),
            uri());
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to parse URI \"%s\". %s."),
            uri(), error->message);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        if (response != GTK_RESPONSE_YES)
        {
            cancelClosing();
            return false;
        }
    }
    xmlNodePtr node = writeManifestFile();
    xmlDocPtr doc = xmlNewDoc(reinterpret_cast<const xmlChar *>("1.0"));
    xmlDocSetRootElement(doc, node);
    if (xmlSaveFormatFile(xmlFileName, doc, 1) == -1)
    {
        xmlErrorPtr error = xmlGetLastError();
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to close project \"%s\". Force to close it?"),
            uri());
        if (error)
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to write project manifest file \"%s\". %s."),
                xmlUri.c_str(), error->message);
        else
            gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to write project manifest file \"%s\"."),
                xmlUri.c_str());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_NO);
        int response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (response != GTK_RESPONSE_YES)
        {
            xmlFreeDoc(doc);
            g_free(xmlFileName);
            cancelClosing();
            return false;
        }
    }
    xmlFreeDoc(doc);
    g_free(xmlFileName);

    // Close the project database.
    int dbError = m_db->close();
    if (dbError)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_YES_NO,
            _("Samoyed failed to close project \"%s\". Force to close it?"),
              uri());
        std::string dbUri(uri());
        dbUri += "/.samoyed/project.db";
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to close project database \"%s\". %s."),
            dbUri.c_str(), db_strerror(dbError));
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

Project *Project::openByDialog()
{
    Project *project = NULL;
    GtkWidget *dialog =
        gtk_file_chooser_dialog_new(
            _("Open Project"),
        Application::instance().currentWindow() ?
        GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
        NULL,
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_OK,
        NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    g_signal_connect(GTK_FILE_CHOOSER(dialog), "selection-changed",
                     G_CALLBACK(checkProjectExists), dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
    {
        char *uri = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(dialog));
        project = Project::open(uri);
        g_free(uri);
    }
    gtk_widget_destroy(dialog);
    return project;
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
}

void Project::destroyEditor(Editor &editor)
{
    editor.destroyInProject();
    if (m_closing && !m_firstEditor)
        finishClosing();
}

void Project::setBuildSystem(const char *buildSystemExtensionId)
{
    if (m_buildSystemExtensionId == buildSystemExtensionId)
        return;
    m_buildSystemExtensionId = buildSystemExtensionId;
    delete m_buildSystem;
    if (strcmp(buildSystemExtensionId, "basic-build-system") == 0)
        m_buildSystem = new BuildSystem(*this);
    else
        m_buildSystem =
            static_cast<BuildSystemsExtensionPoint &>(Application::instance().
                extensionPointManager().extensionPoint(BUILD_SYSTEMS)).
                activateBuildSystem(buildSystemExtensionId, *this);
}

}
