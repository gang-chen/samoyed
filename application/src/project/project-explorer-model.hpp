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
        TYPE_COLUMN,
        FLAGS_COLUMN,
        N_COLUMNS
    };

    enum Type
    {
        TYPE_PROJECT,
        TYPE_DIRECTORY,
        TYPE_SOURCE_FILE,
        TYPE_HEADER_FILE,
        TYPE_GENERIC_FILE,
        TYPE_STATIC_LIBRARY,
        TYPE_SHARED_LIBRARY,
        TYPE_PROGRAM,
        TYPE_GENERIC_TARGET,
        TYPE_DUMMY,
        N_TYPES
    };

    enum Flag
    {
        FLAG_IS_LOADED  = 1,
        FLAG_IS_LOADING = 1 << 1
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
