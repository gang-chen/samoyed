// Project explorer.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_EXPLORER_HPP
#define SMYD_PROJECT_EXPLORER_HPP

#include "pane.hpp"
#include "../utilities/misc.hpp"

namespace Samoyed
{

class Project;

class ProjectExplorer: public Pane
{
public:
    virtual bool close();

    Project *findProject(const char *uri);
    const Project *findProject(const char *uri) const;

    void addProject(Project &project);

    void removeProject(Project &project);

    void onProjectClosed();

    Project *projects() { return m_firstProject; }
    const Project *projects() const { return m_firstProject; }

private:
    typedef std::map<ComparablePointer<const char *>, Project*> ProjectTable;

    bool m_closing;

    ProjectTable m_projectTable;
    Project *m_firstProject;
    Project *m_lastProject;
};

}

#endif
