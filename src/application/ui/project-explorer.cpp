// Project explorer.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_HPP
# include <config.h>
#endif
#include "project-explorer.hpp"
#include "project.hpp"
#include <utility>
#include <map>

namespace Samoyed
{

ProjectExplorer::ProjectExplorer():
    Pane(TYPE_PROJECT_EXPLORER),
    m_closing(false),
    m_firstProject(NULL),
    m_lastProject(NULL)
{
}

ProjectExplorer::~ProjectExplorer()
{
}

void ProjectExplorer::close()
{
    setClosing(true);
    for (Project *project = m_firstProject, *next; project; project = next)
    {
        next = project->next();
        if (!project->close())
        {
            setClosing(false);
            return;
        }
    }
}

Project *ProjectExplorer::findProject(const char *uri)
{
    ProjectTable::const_iterator it = m_projectTable.find(uri);
    if (it == m_projectTable.end())
        return NULL;
    return it->second;
}

const Project *ProjectExplorer::findProject(const char *uri) const
{
    ProjectTable::const_iterator it = m_projectTable.find(uri);
    if (it == m_projectTable.end())
        return NULL;
    return it->second;
}

void ProjectExplorer::addProject(Project &project)
{
    m_projectTable.insert(std::make_pair(project.uri(), &project));
    project.addToList(m_firstProject, m_lastProject);
}

void ProjectExplorer::removeProject(Project &project)
{
    m_projectTable.erase(project.uri());
    project.removeFromList(m_firstProject, m_lastProject);
}

void ProjectExplorer::onProjectClosed()
{
    if (closing() && !m_firstProject)
        delete this;
}

}
