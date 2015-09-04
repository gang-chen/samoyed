// Build system.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "build-system.hpp"
#include "configuration.hpp"
#include "build-system-file.hpp"
#include "build-systems-extension-point.hpp"
#include "build-process.hpp"
#include "compilation-options-collector.hpp"
#include "project/project.hpp"
#include "plugin/extension-point-manager.hpp"
#include "window/window.hpp"
#include "application.hpp"
#include <string.h>
#include <map>
#include <list>
#include <string>
#include <libxml/tree.h>
#include <glib/gi18n.h>

#define BUILD_SYSTEM "build-system"
#define EXTENSION_ID "extension-id"
#define CONFIGURATIONS "configurations"
#define CONFIGURATION "configuration"
#define ACTIVE_CONFIGURATION "active-configuration"
#define BUILD_SYSTEMS "build-systems"

namespace
{

void onConfigurationNameEntryChanged(GtkEntry *entry, GtkDialog *dialog)
{
    if (*gtk_entry_get_text(entry) != '\0')
        gtk_widget_set_sensitive(
            gtk_dialog_get_widget_for_response(dialog,
                                               GTK_RESPONSE_ACCEPT),
            TRUE);
    else
        gtk_widget_set_sensitive(
            gtk_dialog_get_widget_for_response(dialog,
                                               GTK_RESPONSE_ACCEPT),
            FALSE);
}

}

namespace Samoyed
{

BuildSystem::BuildSystem(Project &project,
                         const char *extensionId):
    m_project(project),
    m_extensionId(extensionId),
    m_activeConfig(NULL),
    m_compOptCollector(NULL)
{
}

BuildSystem::~BuildSystem()
{
    std::map<std::string, BuildProcess *>::iterator buildJobIt;
    while ((buildJobIt = m_buildJobs.begin()) != m_buildJobs.end())
        stopBuild(buildJobIt->first.c_str());

    if (m_compOptCollector)
    {
        m_compOptCollector->stop();
        delete m_compOptCollector;
    }

    ConfigurationTable::iterator configIt;
    while ((configIt = m_configurations.begin()) != m_configurations.end())
    {
        Configuration *config = configIt->second;
        m_configurations.erase(configIt);
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
                            if (!buildSystem->m_configurations.insert(
                                    std::make_pair(config->name(), config)).
                                    second)
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
                    ConfigurationTable::iterator it =
                        buildSystem->m_configurations.find(value);
                    if (it != buildSystem->m_configurations.end())
                        buildSystem->m_activeConfig = it->second;
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

bool BuildSystem::setup()
{
    if (!activeConfiguration())
        createConfiguration();
    return true;
}

BuildSystemFile *BuildSystem::createFile(int type) const
{
    return new BuildSystemFile;
}

bool BuildSystem::canConfigure() const
{
    const Configuration *config = activeConfiguration();
    if (!config || *config->configureCommands() == '\0')
        return false;
    if (m_buildJobs.find(config->name()) != m_buildJobs.end())
        return false;
    return true;
}

bool BuildSystem::configure()
{
    Configuration *config = activeConfiguration();
    if (!config)
        return false;
    if (m_buildJobs.find(config->name()) != m_buildJobs.end())
        return false;
    BuildProcess *job = new BuildProcess(*this,
                                         config->name(),
                                         config->configureCommands(),
                                         BuildProcess::ACTION_CONFIGURE);
    m_buildJobs.insert(std::make_pair(config->name(), job));
    if (job->run())
        return true;
    m_buildJobs.erase(config->name());
    delete job;
    return false;
}

bool BuildSystem::canBuild() const
{
    const Configuration *config = activeConfiguration();
    if (!config || *config->buildCommands() == '\0')
        return false;
    if (m_buildJobs.find(config->name()) != m_buildJobs.end())
        return false;
    return true;
}

bool BuildSystem::build()
{
    Configuration *config = activeConfiguration();
    if (!config)
        return false;
    if (m_buildJobs.find(config->name()) != m_buildJobs.end())
        return false;
    BuildProcess *job = new BuildProcess(*this,
                                         config->name(),
                                         config->buildCommands(),
                                         BuildProcess::ACTION_BUILD);
    m_buildJobs.insert(std::make_pair(config->name(), job));
    if (job->run())
        return true;
    m_buildJobs.erase(config->name());
    delete job;
    return false;
}

bool BuildSystem::canInstall() const
{
    const Configuration *config = activeConfiguration();
    if (!config || *config->installCommands() == '\0')
        return false;
    if (m_buildJobs.find(config->name()) != m_buildJobs.end())
        return false;
    return true;
}

bool BuildSystem::install()
{
    Configuration *config = activeConfiguration();
    if (!config)
        return false;
    if (m_buildJobs.find(config->name()) != m_buildJobs.end())
        return false;
    BuildProcess *job = new BuildProcess(*this,
                                         config->name(),
                                         config->installCommands(),
                                         BuildProcess::ACTION_INSTALL);
    m_buildJobs.insert(std::make_pair(config->name(), job));
    if (job->run())
        return true;
    m_buildJobs.erase(config->name());
    delete job;
    return false;
}

bool BuildSystem::collectCompilationOptions()
{
    Configuration *config = activeConfiguration();
    if (!config)
        return false;
    m_compOptCollector =
        new CompilationOptionsCollector(*this,
                                        config->dryBuildCommands());
    if (m_compOptCollector->run())
        return true;
    delete m_compOptCollector;
    m_compOptCollector = NULL;
    return false;
}

void BuildSystem::stopBuild(const char *configName)
{
    std::map<std::string, BuildProcess *>::iterator it =
        m_buildJobs.find(configName);
    if (it != m_buildJobs.end())
    {
        it->second->stop();
        delete it->second;
        m_buildJobs.erase(it);
    }
}

void BuildSystem::onBuildFinished(const char *configName)
{
    std::map<std::string, BuildProcess *>::iterator it =
        m_buildJobs.find(configName);
    if (it != m_buildJobs.end())
    {
        delete it->second;
        m_buildJobs.erase(it);
    }
}

void BuildSystem::onCompilationOptionsCollectorFinished()
{
    delete m_compOptCollector;
    m_compOptCollector = NULL;
}

Configuration *BuildSystem::createConfiguration()
{
    Configuration *config = new Configuration;
    defaultConfiguration(*config);

    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "configuration-creator-dialog.xml";
    GtkBuilder *builder = gtk_builder_new_from_file(uiFile.c_str());
    GtkDialog *dialog =
        GTK_DIALOG(gtk_builder_get_object(builder,
                                          "configuration-creator-dialog"));
    GtkEntry *nameEntry =
        GTK_ENTRY(gtk_builder_get_object(builder, "name-entry"));
    g_signal_connect(GTK_EDITABLE(nameEntry), "changed",
                     G_CALLBACK(onConfigurationNameEntryChanged), dialog);
    GtkEntry *configCommandsEntry =
        GTK_ENTRY(gtk_builder_get_object(builder, "configure-commands-entry"));
    gtk_entry_set_text(configCommandsEntry, config->configureCommands());
    GtkEntry *buildCommandsEntry =
        GTK_ENTRY(gtk_builder_get_object(builder, "build-commands-entry"));
    gtk_entry_set_text(buildCommandsEntry, config->buildCommands());
    GtkEntry *installCommandsEntry =
        GTK_ENTRY(gtk_builder_get_object(builder, "install-commands-entry"));
    gtk_entry_set_text(installCommandsEntry, config->installCommands());
    GtkEntry *dryBuildCommandsEntry =
        GTK_ENTRY(gtk_builder_get_object(builder, "dry-build-commands-entry"));
    gtk_entry_set_text(dryBuildCommandsEntry, config->dryBuildCommands());

    if (Application::instance().currentWindow())
    {
        gtk_window_set_transient_for(
            GTK_WINDOW(dialog),
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    }

    for (;;)
    {
        if (gtk_dialog_run(dialog) != GTK_RESPONSE_ACCEPT)
        {
            delete config;
            config = NULL;
            break;
        }

        config->setName(gtk_entry_get_text(nameEntry));
        config->setConfigureCommands(gtk_entry_get_text(configCommandsEntry));
        config->setBuildCommands(gtk_entry_get_text(buildCommandsEntry));
        config->setInstallCommands(gtk_entry_get_text(installCommandsEntry));
        config->setDryBuildCommands(gtk_entry_get_text(dryBuildCommandsEntry));
        if (configurations().insert(std::make_pair(config->name(), config)).
                second)
            break;

        GtkWidget *diag = gtk_message_dialog_new(
            GTK_WINDOW(dialog),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create configuration \"%s\" for project "
              "\"%s\" because configuration \"%s\" already exists."),
            config->name(),
            project().uri(),
            config->name());
        gtk_dialog_run(GTK_DIALOG(diag));
        gtk_widget_destroy(diag);

        gtk_widget_grab_focus(GTK_WIDGET(nameEntry));
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
    g_object_unref(builder);

    if (config)
        m_activeConfig = config;
    return config;
}

void BuildSystem::defaultConfiguration(Configuration &config) const
{
}

}
