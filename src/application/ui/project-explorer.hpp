// Project explorer.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_EXPLORER_HPP
#define SMYD_PROJECT_EXPLORER_HPP

#include "pane.hpp"
#include "../utilities/misc.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

class Project;

class ProjectExplorer: public Pane
{
public:
    ProjectExplorer();

    virtual bool close();

    virtual GtkWidget *gtkWidget() const { return m_notebook; }

    Project *findProject(const char *uri);
    const Project *findProject(const char *uri) const;

    void addProject(Project &project);

    void removeProject(Project &project);

    void onProjectClosed();

    Project *projects() { return m_firstProject; }
    const Project *projects() const { return m_firstProject; }

protected:
    virtual ~ProjectExplorer();

private:
    typedef std::map<ComparablePointer<const char *>, Project*> ProjectTable;

    ProjectTable m_projectTable;
    Project *m_firstProject;
    Project *m_lastProject;

    GtkWidget *m_notebook;
};

}

#endif
