// Project build configuration.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_CONFIGURATION_HPP
#define SMYD_CONFIGURATION_HPP

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
    const char *dryBuildCommands() const { return m_dryBuildCommands.c_str(); }
    void setDryBuildCommands(const char *commands)
    { m_dryBuildCommands = commands; }

    bool readXmlElement(xmlNodePtr node, std::list<std::string> *errors);
    xmlNodePtr writeXmlElement() const;

private:
    std::string m_name;
    std::string m_configCommands;
    std::string m_buildCommands;
    std::string m_installCommands;
    std::string m_dryBuildCommands;
};

}

#endif
