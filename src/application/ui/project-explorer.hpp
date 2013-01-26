// Project explorer.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_EXPLORER_HPP
#define SMYD_PROJECT_EXPLORER_HPP

#include <gtk/gtk.h>

namespace Samoyed
{

class Project;

class ProjectExplorer
{
public:
    ProjectExplorer();

    bool close() { return true; }

    GtkWidget *gtkWidget() const { return m_tree; }

protected:
    virtual ~ProjectExplorer();

private:
    GtkWidget *m_tree;
};

}

#endif
