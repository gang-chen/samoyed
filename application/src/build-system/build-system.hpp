// Build system.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_SYSTEM_HPP
#define SMYD_BUILD_SYSTEM_HPP

#include "project/project-file.hpp"
#include "utilities/miscellaneous.hpp"
#include <map>
#include <list>
#include <string>
#include <boost/utility.hpp>
#include <libxml/tree.h>

namespace Samoyed
{

class Project;
class Configuration;

class BuildSystem: public boost::noncopyable
{
public:
    typedef std::map<ComparablePointer<const char>, Configuration *>
        ConfigurationTable;

    static BuildSystem *create(Project &project, const char *extensionId);

    virtual ~BuildSystem();

    /**
     * Copy template files to a newly created project.
     */
    virtual bool copyTemplateFiles() { return true; }

    /**
     * Create default configurations for a newly created project.
     */
    virtual void createDefaultConfigurations() {}

    /**
     * Import an existing file to a project.
     */
    virtual bool importFile(const char *uri) { return true; }

    virtual bool addProjectFile(ProjectFile &projectFile);

    virtual bool removeProjectFile(const char *uri);

    bool configure();
    bool build();
    bool install();
    bool collectCompilationOptions();

    Project &project() { return m_project; }
    const Project &project() const { return m_project; }

    ConfigurationTable &configurations() { return m_configurations; }
    const ConfigurationTable &configurations() const
    { return m_configurations; }

    Configuration *activeConfiguration() { return m_activeConfig; }
    const Configuration *activeConfiguration() const { return m_activeConfig; }
    void setActiveConfiguration(Configuration *config)
    { m_activeConfig = config; }

    virtual BuildSystemFile *
    createBuildSystemFile(ProjectFile::Type type) const;

    static BuildSystem *readXmlElement(Project &project,
                                       xmlNodePtr node,
                                       std::list<std::string> *errors);
    xmlNodePtr writeXmlElement() const;

protected:
    BuildSystem(Project &project, const char *extensionId);

private:
    Project &m_project;

    const std::string m_extensionId;

    ConfigurationTable m_configurations;

    Configuration *m_activeConfig;
};

}

#endif
