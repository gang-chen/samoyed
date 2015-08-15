// Project build configuration.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "configuration.hpp"
#include <string.h>
#include <list>
#include <string>
#include <libxml/tree.h>

#define CONFIGURATION "configuration"
#define NAME "name"
#define CONFIGURE_COMMANDS "configure-commands"
#define BUILD_COMMANDS "build-commands"
#define INSTALL_COMMANDS "install-commands"
#define DRY_BUILD_COMMANDS "dry-build-commands"

namespace Samoyed
{

bool Configuration::readXmlElement(xmlNodePtr node,
                                   std::list<std::string> *errors)
{
    char *value;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   NAME) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            m_name = value;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CONFIGURE_COMMANDS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            m_configCommands = value;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        BUILD_COMMANDS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            m_buildCommands = value;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        INSTALL_COMMANDS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            m_installCommands = value;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        DRY_BUILD_COMMANDS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            m_dryBuildCommands = value;
        }
    }
    return true;
}

xmlNodePtr Configuration::writeXmlElement() const
{
    xmlNodePtr node =
        xmlNewNode(NULL,
                   reinterpret_cast<const xmlChar *>(CONFIGURATION));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(NAME),
                    reinterpret_cast<const xmlChar *>(name()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CONFIGURE_COMMANDS),
                    reinterpret_cast<const xmlChar *>(configureCommands()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(BUILD_COMMANDS),
                    reinterpret_cast<const xmlChar *>(buildCommands()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(INSTALL_COMMANDS),
                    reinterpret_cast<const xmlChar *>(installCommands()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(DRY_BUILD_COMMANDS),
                    reinterpret_cast<const xmlChar *>(dryBuildCommands()));
}

}
