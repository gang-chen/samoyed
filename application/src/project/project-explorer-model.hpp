// Model of project explorers.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_EXPLORER_MODEL_HPP
#define SMYD_PROJECT_EXPLORER_MODEL_HPP

#include "utilities/miscellaneous.hpp"
#include <map>
#include <boost/signals2/connection.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class Project;
class ProjectFile;

class ProjectExplorerModel
{
public:
    enum Column
    {
        NAME_COLUMN,
        ICON_COLUMN,
        FLAGS_COLUMN,
        N_COLUMNS
    };

    enum Flag
    {
        FLAG_IS_DIRECTORY   = 1,
        FLAG_IS_DUMMY       = 1 << 1,
        FLAG_IS_LOADED      = 1 << 2,
        FLAG_IS_LOADING     = 1 << 3
    };

    ProjectExplorerModel();
    ~ProjectExplorerModel();

    GtkTreeModel *model()
    { return GTK_TREE_MODEL(m_store); }

    void onRowExpanded(GtkTreeIter *iter);

private:
    struct Connections
    {
        boost::signals2::connection closed;
        boost::signals2::connection fileAdded;
        boost::signals2::connection fileRemoved;
    };

    typedef std::map<ComparablePointer<const char>, Connections>
        ProjectConnectionsTable;

    void onProjectOpened(Project &project);
    void onProjectClosed(Project &project);

    void onProjectFileAdded(Project &project,
                            const char *uri,
                            const ProjectFile &data);
    void onProjectFileRemoved(Project &project,
                              const char *uri);

    GtkTreeStore *m_store;

    boost::signals2::connection m_openedConn;
    ProjectConnectionsTable m_projConnsTable;
};

}

#endif
