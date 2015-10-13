// Project build configuration.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "configuration.hpp"
#include <string.h>
#include <list>
#include <string>
#include <boost/lexical_cast.hpp>
#include <libxml/tree.h>
#include <glib.h>
#include <glib/gi18n.h>

#define CONFIGURATION "configuration"
#define NAME "name"
#define CONFIGURE_COMMANDS "configure-commands"
#define BUILD_COMMANDS "build-commands"
#define INSTALL_COMMANDS "install-commands"
#define COMPILER "compiler"
#define COLLECT_COMPILER_OPTIONS_AUTOMATICALLY \
    "collect-compiler-options-automatically"
#define COMPILER_OPTIONS "compiler-options"

namespace Samoyed
{

bool Configuration::readXmlElement(xmlNodePtr node,
                                   std::list<std::string> *errors)
{
    char *value, *cp;
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
                        COMPILER) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
            	m_compiler = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        COLLECT_COMPILER_OPTIONS_AUTOMATICALLY) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_autoCompilerOptions = boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                            child->line,
                            value,
                            COLLECT_COMPILER_OPTIONS_AUTOMATICALLY,
                            error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        COMPILER_OPTIONS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
            	m_compilerOptions = value;
                xmlFree(value);
            }
        }
    }
    return true;
}

xmlNodePtr Configuration::writeXmlElement() const
{
    std::string str;
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
                    reinterpret_cast<const xmlChar *>(COMPILER),
                    reinterpret_cast<const xmlChar *>(compiler()));
    str = boost::lexical_cast<std::string>(m_autoCompilerOptions);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(
                        COLLECT_COMPILER_OPTIONS_AUTOMATICALLY),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(COMPILER_OPTIONS),
                    reinterpret_cast<const xmlChar *>(compilerOptions()));
    return node;
}

}
