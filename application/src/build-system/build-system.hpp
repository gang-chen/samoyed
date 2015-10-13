// Build system.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_BUILD_SYSTEM_HPP
#define SMYD_BUILD_SYSTEM_HPP

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
class BuildSystemFile;
class Builder;

class BuildSystem: public boost::noncopyable
{
public:
    static BuildSystem *create(Project &project, const char *extensionId);

    virtual ~BuildSystem();

    /**
     * Setup the build system for a newly created project, including creating
     * build-system-specific files, and importing existing files, if any.
     */
    virtual bool setup();

    /**
     * Import an existing file, or recursively all the files in a directory, to
     * a project.
     */
    virtual bool importFile(const char *uri) { return true; }

    virtual BuildSystemFile *createFile(int type) const;

    virtual bool addFile(const char *uri, const BuildSystemFile &data)
    { return true; }

    virtual bool removeFile(const char *uri)
    { return true; }

    bool canConfigure() const;
    bool configure();

    bool canBuild() const;
    bool build();

    bool canInstall() const;
    bool install();

    void stopBuild(const char *configName);

    void onBuildFinished(const char *configName);

    Project &project() { return m_project; }
    const Project &project() const { return m_project; }

    Configuration *findConfiguration(const char *name);
    const Configuration *findConfiguration(const char *name) const;

    bool addConfiguration(Configuration &config);
    bool removeConfiguration(Configuration &config);

    Configuration *configurations() { return m_firstConfig; }
    const Configuration *configurations() const { return m_firstConfig; }

    Configuration *activeConfiguration() { return m_activeConfig; }
    const Configuration *activeConfiguration() const { return m_activeConfig; }
    void setActiveConfiguration(Configuration *config)
    { m_activeConfig = config; }

    virtual Configuration defaultConfiguration() const;

    static BuildSystem *readXmlElement(Project &project,
                                       xmlNodePtr node,
                                       std::list<std::string> *errors);
    xmlNodePtr writeXmlElement() const;

protected:
    BuildSystem(Project &project, const char *extensionId);

private:
    typedef std::map<ComparablePointer<const char>, Configuration *>
        ConfigurationTable;

    typedef std::map<ComparablePointer<const char>, Builder *> BuilderTable;

    Project &m_project;

    const std::string m_extensionId;

    ConfigurationTable m_configTable;
    Configuration *m_firstConfig;
    Configuration *m_lastConfig;

    Configuration *m_activeConfig;

    BuilderTable m_builders;
};

}

#endif
