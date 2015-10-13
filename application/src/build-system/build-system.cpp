// Build system.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "build-system.hpp"
#include "configuration.hpp"
#include "build-system-file.hpp"
#include "build-systems-extension-point.hpp"
#include "builder.hpp"
#include "project/project.hpp"
#include "plugin/extension-point-manager.hpp"
#include "application.hpp"
#include <string.h>
#include <map>
#include <list>
#include <string>
#include <utility>
#include <libxml/tree.h>
#include <glib/gi18n.h>

#define BUILD_SYSTEM "build-system"
#define EXTENSION_ID "extension-id"
#define CONFIGURATIONS "configurations"
#define CONFIGURATION "configuration"
#define ACTIVE_CONFIGURATION "active-configuration"
#define BUILD_SYSTEMS "build-systems"

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
}

BuildSystem::~BuildSystem()
{
    BuilderTable::iterator it;
    while ((it = m_builders.begin()) != m_builders.end())
        stopBuild(it->first);

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

bool BuildSystem::setup()
{
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

void BuildSystem::onBuildFinished(const char *configName)
{
    BuilderTable::iterator it = m_builders.find(configName);
    if (it != m_builders.end())
    {
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

}
