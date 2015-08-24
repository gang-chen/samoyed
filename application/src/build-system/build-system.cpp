// Build system.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "build-system.hpp"
#include "configuration.hpp"
#include "build-system-file.hpp"
#include "build-systems-extension-point.hpp"
#include "project/project-file.hpp"
#include "plugin/extension-point-manager.hpp"
#include "application.hpp"
#include <string.h>
#include <map>
#include <list>
#include <string>
#include <libxml/tree.h>

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
    m_activeConfig(NULL)
{
}

BuildSystem::~BuildSystem()
{
    ConfigurationTable::iterator it;
    while ((it = m_configurations.begin()) != m_configurations.end())
    {
        Configuration *config = it->second;
        m_configurations.erase(it);
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

BuildSystemFile *
BuildSystem::createBuildSystemFile(ProjectFile::Type type) const
{
    return new BuildSystemFile();
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

bool BuildSystem::addProjectFile(ProjectFile &projectFile)
{
    return true;
}

bool BuildSystem::removeProjectFile(const char *uri)
{
    return true;
}

}
