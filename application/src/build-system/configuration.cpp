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
#define CLEAN_COMMANDS "clean-commands"
#define C_COMPILER "c-compiler"
#define CPP_COMPILER "cpp-compiler"
#define C_COMPILER_OPTIONS "c-compiler-options"
#define CPP_COMPILER_OPTIONS "cpp-compiler-options"

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
            if (value)
            {
                m_name = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CONFIGURE_COMMANDS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_configCommands = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        BUILD_COMMANDS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_buildCommands = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        INSTALL_COMMANDS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_installCommands = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CLEAN_COMMANDS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_cleanCommands = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        C_COMPILER) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_cCompiler = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CPP_COMPILER) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_cppCompiler = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        C_COMPILER_OPTIONS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_cCompilerOptions = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CPP_COMPILER_OPTIONS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_cppCompilerOptions = value;
                xmlFree(value);
            }
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
                    reinterpret_cast<const xmlChar *>(CLEAN_COMMANDS),
                    reinterpret_cast<const xmlChar *>(cleanCommands()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(C_COMPILER),
                    reinterpret_cast<const xmlChar *>(cCompiler()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CPP_COMPILER),
                    reinterpret_cast<const xmlChar *>(cppCompiler()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(C_COMPILER_OPTIONS),
                    reinterpret_cast<const xmlChar *>(cCompilerOptions()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CPP_COMPILER_OPTIONS),
                    reinterpret_cast<const xmlChar *>(cppCompilerOptions()));
    return node;
}

}
