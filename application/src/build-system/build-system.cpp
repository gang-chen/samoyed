// Build system.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "build-system.hpp"
#include "configuration.hpp"
#include "build-system-file.hpp"
#include "build-systems-extension-point.hpp"
#include "build-log-view.hpp"
#include "build-log-view-group.hpp"
#include "builder.hpp"
#include "compiler-options-collector.hpp"
#include "project/project.hpp"
#include "project/project-file.hpp"
#include "plugin/extension-point-manager.hpp"
#include "window/window.hpp"
#include "utilities/miscellaneous.hpp"
#include "application.hpp"
#include <string.h>
#include <map>
#include <list>
#include <set>
#include <string>
#include <utility>
#include <libxml/tree.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#define BUILD_SYSTEM "build-system"
#define EXTENSION_ID "extension-id"
#define CONFIGURATIONS "configurations"
#define CONFIGURATION "configuration"
#define ACTIVE_CONFIGURATION "active-configuration"
#define BUILD_SYSTEMS "build-systems"

namespace
{

const char *SOURCE_FILE_NAME_SUFFIXES[] =
{
    "c",
    "cc",
    "cp",
    "cxx",
    "cpp",
    "CPP",
    "c++",
    "C",
#ifdef OS_WINDOWS
    // Mixed case letters are not allowed.
    "CC",
    "CP",
    "CXX",
    "C++",
#endif
    NULL
};

const char *HEADER_FILE_NAME_SUFFIXES[] =
{
    "h",
    "hh",
    "hpp",
    "H",
#ifdef OS_WINDOWS
    // Mixed case letters are not allowed.
    "HH",
    "HPP",
#endif
    NULL
};

const char *BUILD_SYSTEM_FILE_NAMES[] =
{
    "GNUmakefile",
    "makefile",
    "Makefile",
    NULL
};

std::set<Samoyed::ComparablePointer<const char> > sourceFileNameSuffixes;
std::set<Samoyed::ComparablePointer<const char> > headerFileNameSuffixes;
std::set<Samoyed::ComparablePointer<const char> > buildSystemFileNames;

}

namespace Samoyed
{

BuildSystem::BuildSystem(Project &project,
                         const char *extensionId):
    m_project(project),
    m_extensionId(extensionId),
    m_firstConfig(NULL),
    m_lastConfig(NULL),
    m_activeConfig(NULL)
{
    if (sourceFileNameSuffixes.empty())
    {
        for (const char **suffix = SOURCE_FILE_NAME_SUFFIXES; *suffix; suffix++)
            sourceFileNameSuffixes.insert(*suffix);
    }
    if (headerFileNameSuffixes.empty())
    {
        for (const char **suffix = HEADER_FILE_NAME_SUFFIXES; *suffix; suffix++)
            headerFileNameSuffixes.insert(*suffix);
    }
    if (buildSystemFileNames.empty())
    {
        for (const char **name = BUILD_SYSTEM_FILE_NAMES; *name; name++)
            buildSystemFileNames.insert(*name);
    }
}

BuildSystem::~BuildSystem()
{
    // Close all the build log views.
    Window *window = Application::instance().currentWindow();
    if (window)
    {
        BuildLogViewGroup *group = window->buildLogViewGroup();
        if (group)
        {
            std::list<BuildLogView *> views;
            group->buildLogViewsForProject(m_project.uri(), views);
            for (std::list<BuildLogView *>::iterator it = views.begin();
                 it != views.end();
                 ++it)
                (*it)->close();
        }
    }

    while (m_firstConfig)
    {
        Configuration *config = m_firstConfig;
        removeConfiguration(*config);
        delete config;
    }
}

BuildSystem *BuildSystem::create(Project &project,
                                 const char *extensionId)
{
    if (strcmp(extensionId, "basic-build-system") == 0)
        return new BuildSystem(project, extensionId);
    return static_cast<BuildSystemsExtensionPoint &>(Application::instance().
        extensionPointManager().extensionPoint(BUILD_SYSTEMS)).
        activateBuildSystem(project, extensionId);
}

BuildSystem *BuildSystem::readXmlElement(Project &project,
                                         xmlNodePtr node,
                                         std::list<std::string> *errors)
{
    BuildSystem *buildSystem = NULL;
    char *value, *cp;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   EXTENSION_ID) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                buildSystem = create(project, value);
                xmlFree(value);
            }
        }
        else if (buildSystem)
        {
            if (strcmp(reinterpret_cast<const char *>(child->name),
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
                            if (!buildSystem->addConfiguration(*config))
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
                    Configuration *config =
                        buildSystem->findConfiguration(value);
                    if (config)
                        buildSystem->setActiveConfiguration(config);
                    xmlFree(value);
                }
            }
        }
    }
    return buildSystem;
}

xmlNodePtr BuildSystem::writeXmlElement() const
{
    xmlNodePtr node =
        xmlNewNode(NULL,
                   reinterpret_cast<const xmlChar *>(BUILD_SYSTEM));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(EXTENSION_ID),
                    reinterpret_cast<const xmlChar *>(m_extensionId.c_str()));
    xmlNodePtr configs =
        xmlNewNode(NULL,
                   reinterpret_cast<const xmlChar *>(CONFIGURATIONS));
    for (ConfigurationTable::const_iterator it = m_configTable.begin();
         it != m_configTable.end();
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

bool BuildSystem::isSourceFile(const char *fileName)
{
    const char *ext = strrchr(fileName, '.');
    if (ext &&
        sourceFileNameSuffixes.find(ext + 1) != sourceFileNameSuffixes.end())
        return true;
    return false;
}

bool BuildSystem::isHeaderFile(const char *fileName)
{
    const char *ext = strrchr(fileName, '.');
    if (ext &&
        headerFileNameSuffixes.find(ext + 1) != headerFileNameSuffixes.end())
        return true;
    return false;
}

bool BuildSystem::isBuildSystemFile(const char *fileName)
{
    if (buildSystemFileNames.find(fileName) != buildSystemFileNames.end())
        return true;
    return false;
}

bool BuildSystem::importDirectory(const char *dirName)
{
    GError *error = NULL;
    GDir *dir;
    dir = g_dir_open(dirName, 0, &error);
    if (!dir)
    {
        char *uri = g_filename_to_uri(dirName, NULL, NULL);
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to import files in directory \"%s\" into project "
              "\"%s\"."),
            uri, project().uri());
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to open directory \"%s\". %s."),
            uri, error->message);
        g_free(uri);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return false;
    }
    bool successful = true;
    std::string name;
    const char *entry;
    while ((entry = g_dir_read_name(dir)) != NULL)
    {
        if (entry[0] == '.')
            continue;
        name = dirName;
        name += G_DIR_SEPARATOR;
        name += entry;
        if (g_file_test(name.c_str(), G_FILE_TEST_IS_REGULAR))
        {
            if (isSourceFile(name.c_str()))
            {
                ProjectFile *file =
                    project().createFile(ProjectFile::TYPE_SOURCE_FILE);
                char *uri = g_filename_to_uri(name.c_str(), NULL, NULL);
                if (!project().addFile(uri, *file, false))
                {
                    successful = false;
                    g_free(uri);
                    break;
                }
                g_free(uri);
            }
            else if (isHeaderFile(name.c_str()))
            {
                ProjectFile *file =
                    project().createFile(ProjectFile::TYPE_HEADER_FILE);
                char *uri = g_filename_to_uri(name.c_str(), NULL, NULL);
                if (!project().addFile(uri, *file, false))
                {
                    successful = false;
                    g_free(uri);
                    break;
                }
                g_free(uri);
            }
            else if (isBuildSystemFile(name.c_str()))
            {
                ProjectFile *file =
                    project().createFile(ProjectFile::TYPE_GENERIC_FILE);
                char *uri = g_filename_to_uri(name.c_str(), NULL, NULL);
                if (!project().addFile(uri, *file, false))
                {
                    successful = false;
                    g_free(uri);
                    break;
                }
                g_free(uri);
            }
        }
        else if (g_file_test(name.c_str(), G_FILE_TEST_IS_DIR))
        {
            if (!importDirectory(name.c_str()))
            {
                successful = false;
                break;
            }
        }
    }
    g_dir_close(dir);
    return successful;
}

bool BuildSystem::setup()
{
    char *fileName = g_filename_from_uri(project().uri(), NULL, NULL);
    if (!importDirectory(fileName))
    {
        g_free(fileName);
        return false;
    }
    g_free(fileName);
    return true;
}

BuildSystemFile *BuildSystem::createFile(int type) const
{
    return new BuildSystemFile;
}

bool BuildSystem::canConfigure() const
{
    if (m_project.closing())
        return false;
    const Configuration *config = activeConfiguration();
    if (!config || *config->configureCommands() == '\0')
        return false;
    if (m_builders.find(config->name()) != m_builders.end())
        return false;
    return true;
}

bool BuildSystem::configure()
{
    if (!canConfigure())
        return false;
    Configuration *config = activeConfiguration();
    Builder *builder = new Builder(*this,
                                   *config,
                                   Builder::ACTION_CONFIGURE);
    m_builders.insert(std::make_pair(config->name(), builder));
    if (builder->run())
        return true;
    m_builders.erase(config->name());
    delete builder;
    return false;
}

bool BuildSystem::canBuild() const
{
    if (m_project.closing())
        return false;
    const Configuration *config = activeConfiguration();
    if (!config || *config->buildCommands() == '\0')
        return false;
    if (m_builders.find(config->name()) != m_builders.end())
        return false;
    return true;
}

bool BuildSystem::build()
{
    if (!canBuild())
        return false;
    Configuration *config = activeConfiguration();
    Builder *builder = new Builder(*this,
                                   *config,
                                   Builder::ACTION_BUILD);
    m_builders.insert(std::make_pair(config->name(), builder));
    if (builder->run())
        return true;
    m_builders.erase(config->name());
    delete builder;
    return false;
}

bool BuildSystem::canInstall() const
{
    if (m_project.closing())
        return false;
    const Configuration *config = activeConfiguration();
    if (!config || *config->installCommands() == '\0')
        return false;
    if (m_builders.find(config->name()) != m_builders.end())
        return false;
    return true;
}

bool BuildSystem::install()
{
    if (!canInstall())
        return false;
    Configuration *config = activeConfiguration();
    Builder *builder = new Builder(*this,
                                   *config,
                                   Builder::ACTION_INSTALL);
    m_builders.insert(std::make_pair(config->name(), builder));
    if (builder->run())
        return true;
    m_builders.erase(config->name());
    delete builder;
    return false;
}

bool BuildSystem::canClean() const
{
    if (m_project.closing())
        return false;
    const Configuration *config = activeConfiguration();
    if (!config || *config->cleanCommands() == '\0')
        return false;
    if (m_builders.find(config->name()) != m_builders.end())
        return false;
    return true;
}

bool BuildSystem::clean()
{
    if (!canClean())
        return false;
    Configuration *config = activeConfiguration();
    Builder *builder = new Builder(*this,
                                   *config,
                                   Builder::ACTION_CLEAN);
    m_builders.insert(std::make_pair(config->name(), builder));
    if (builder->run())
        return true;
    m_builders.erase(config->name());
    delete builder;
    return false;
}

void BuildSystem::stopBuild(const char *configName)
{
    BuilderTable::iterator it = m_builders.find(configName);
    if (it != m_builders.end())
    {
        it->second->stop();
        delete it->second;
        m_builders.erase(it);
    }
}

void BuildSystem::onBuildFinished(const char *configName,
                                  const char *compilerOptsFileName)
{
    BuilderTable::iterator it = m_builders.find(configName);
    if (it != m_builders.end())
    {
        // Start the compiler options collector.
        if (compilerOptsFileName)
        {
            boost::shared_ptr<CompilerOptionsCollector>
                compilerOptsCollector(new
                    CompilerOptionsCollector(
                        Application::instance().scheduler(),
                        Worker::PRIORITY_IDLE,
                        project(),
                        compilerOptsFileName));
            m_compilerOptsCollectors.push_back(compilerOptsCollector);
            compilerOptsCollector->addFinishedCallbackInMainThread(
                boost::bind(onCompilerOptionsCollectorFinished, this, _1));
            compilerOptsCollector->addCanceledCallbackInMainThread(
                boost::bind(onCompilerOptionsCollectorCanceled, this, _1));
            compilerOptsCollector->submit(compilerOptsCollector);
        }

        delete it->second;
        m_builders.erase(it);
    }
}

Configuration BuildSystem::defaultConfiguration() const
{
    return Configuration();
}

Configuration *BuildSystem::findConfiguration(const char *name)
{
    ConfigurationTable::const_iterator it = m_configTable.find(name);
    if (it == m_configTable.end())
        return NULL;
    return it->second;
}

const Configuration *BuildSystem::findConfiguration(const char *name) const
{
    ConfigurationTable::const_iterator it = m_configTable.find(name);
    if (it == m_configTable.end())
        return NULL;
    return it->second;
}

bool BuildSystem::addConfiguration(Configuration &config)
{
    if (!m_configTable.insert(std::make_pair(config.name(), &config)).second)
        return false;
    config.addToList(m_firstConfig, m_lastConfig);
    if (!activeConfiguration())
        setActiveConfiguration(&config);
    return true;
}

bool BuildSystem::removeConfiguration(Configuration &config)
{
    if (m_builders.find(config.name()) != m_builders.end())
        return false;
    if (!m_configTable.erase(config.name()))
        return false;
    config.removeFromList(m_firstConfig, m_lastConfig);
    if (activeConfiguration() == &config)
        setActiveConfiguration(m_firstConfig);
    return true;
}

void BuildSystem::stopAllWorkers(
    const boost::function<void (BuildSystem &)> &callback)
{
    // Stop all the builders.
    BuilderTable::iterator it;
    while ((it = m_builders.begin()) != m_builders.end())
        stopBuild(it->first);

    // Cancel all the compiler options collectors.
    m_allWorkersStoppedCallback = callback;
    if (m_compilerOptsCollectors.empty() && m_allWorkersStoppedCallback)
    {
        g_idle_add_full(G_PRIORITY_HIGH,
                        onAllWorkersStoppedDeferred,
                        this,
                        NULL);
        return;
    }
    for (std::list<boost::shared_ptr<CompilerOptionsCollector> >::const_iterator
            it = m_compilerOptsCollectors.begin();
         it != m_compilerOptsCollectors.end();
         ++it)
        (*it)->cancel(*it);
}

void BuildSystem::onCompilerOptionsCollectorFinished(
    const boost::shared_ptr<Worker> &worker)
{
    for (std::list<boost::shared_ptr<CompilerOptionsCollector> >::iterator
            it = m_compilerOptsCollectors.begin();
         it != m_compilerOptsCollectors.end();
         ++it)
    {
        if (*it == worker)
        {
            m_compilerOptsCollectors.erase(it);
            if (m_compilerOptsCollectors.empty() && m_allWorkersStoppedCallback)
                m_allWorkersStoppedCallback(*this);
            break;
        }
    }
}

void BuildSystem::onCompilerOptionsCollectorCanceled(
    const boost::shared_ptr<Worker> &worker)
{
    for (std::list<boost::shared_ptr<CompilerOptionsCollector> >::iterator
            it = m_compilerOptsCollectors.begin();
         it != m_compilerOptsCollectors.end();
         ++it)
    {
        if (*it == worker)
        {
            m_compilerOptsCollectors.erase(it);
            if (m_compilerOptsCollectors.empty() && m_allWorkersStoppedCallback)
                m_allWorkersStoppedCallback(*this);
            break;
        }
    }
}

gboolean BuildSystem::onAllWorkersStoppedDeferred(gpointer buildSystem)
{
    BuildSystem *b = static_cast<BuildSystem *>(buildSystem);
    b->m_allWorkersStoppedCallback(*b);
    return FALSE;
}

}
