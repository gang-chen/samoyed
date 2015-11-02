// Project build configuration.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_CONFIGURATION_HPP
#define SMYD_CONFIGURATION_HPP

#include "utilities/miscellaneous.hpp"
#include <list>
#include <string>
#include <libxml/tree.h>

namespace Samoyed
{

class Configuration
{
public:
    const char *name() const { return m_name.c_str(); }
    void setName(const char *name) { m_name = name; }
    const char *configureCommands() const { return m_configCommands.c_str(); }
    void setConfigureCommands(const char *commands)
    { m_configCommands = commands; }
    const char *buildCommands() const { return m_buildCommands.c_str(); }
    void setBuildCommands(const char *commands)
    { m_buildCommands = commands; }
    const char *installCommands() const { return m_installCommands.c_str(); }
    void setInstallCommands(const char *commands)
    { m_installCommands = commands; }
    const char *cleanCommands() const { return m_cleanCommands.c_str(); }
    void setCleanCommands(const char *commands)
    { m_cleanCommands = commands; }
    const char *cCompiler() const { return m_cCompiler.c_str(); }
    void setCCompiler(const char *compiler)
    { m_cCompiler = compiler; }
    const char *cppCompiler() const { return m_cppCompiler.c_str(); }
    void setCppCompiler(const char *compiler)
    { m_cppCompiler = compiler; }
    const char *cCompilerOptions() const { return m_cCompilerOptions.c_str(); }
    void setCCompilerOptions(const char *options)
    { m_cCompilerOptions = options; }
    const char *cppCompilerOptions() const
    { return m_cppCompilerOptions.c_str(); }
    void setCppCompilerOptions(const char *options)
    { m_cppCompilerOptions = options; }

    bool readXmlElement(xmlNodePtr node, std::list<std::string> *errors);
    xmlNodePtr writeXmlElement() const;

private:
    std::string m_name;
    std::string m_configCommands;
    std::string m_buildCommands;
    std::string m_installCommands;
    std::string m_cleanCommands;
    std::string m_cCompiler;
    std::string m_cppCompiler;
    std::string m_cCompilerOptions;
    std::string m_cppCompilerOptions;

    SAMOYED_DEFINE_DOUBLY_LINKED(Configuration)
};

}

#endif
