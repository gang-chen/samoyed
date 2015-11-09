// Model of project explorers.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-explorer-model.hpp"
#include "project.hpp"
#include "application.hpp"
#include <string.h>
#include <string>
#include <boost/bind.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

namespace Samoyed
{

ProjectExplorerModel::ProjectExplorerModel()
{
    m_store = gtk_tree_store_new(N_COLUMNS,
                                 G_TYPE_STRING,
                                 G_TYPE_STRING,
                                 G_TYPE_INT);

    // Load the existing projects.
    for (Project *project = Application::instance().projects();
         project;
         project = project->next())
        onProjectOpened(*project);

    m_openedConn = Project::addOpenedCallback(boost::bind(onProjectOpened,
                                                          this,
                                                          _1));
}

ProjectExplorerModel::~ProjectExplorerModel()
{
    // Unload the existing projects.
    ProjectConnectionsTable::iterator it;
    while ((it = m_projConnsTable.begin()) != m_projConnsTable.end())
        onProjectClosed(*Application::instance().findProject(it->first));

    m_openedConn.disconnect();

    g_object_unref(m_store);
}

void ProjectExplorerModel::onProjectOpened(Project &project)
{
    char *projFileName = g_filename_from_uri(project.uri(), NULL, NULL);
    char *projBaseName = g_path_get_basename(projFileName);
    std::string name = projBaseName;
    name += " (";
    int len = name.length();
    name += project.uri();
    name += ")";
    g_free(projFileName);
    g_free(projBaseName);

    GtkTreeIter it;
    bool added = false;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(m_store), &it))
    {
        do
        {
            char *nm;
            gtk_tree_model_get(GTK_TREE_MODEL(m_store), &it,
                               NAME_COLUMN, &nm, -1);
            int cmp = strncmp(nm, name.c_str(), len);
            if (cmp == 0)
                cmp = strcmp(nm, name.c_str());
            g_free(nm);
            if (cmp > 0)
            {
                GtkTreeIter sibling(it);
                gtk_tree_store_insert_before(m_store, &it, NULL, &sibling);
                added = true;
                break;
            }
        }
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(m_store), &it));
    }
    if (!added)
        gtk_tree_store_prepend(m_store, &it, NULL);
    gtk_tree_store_set(m_store, &it,
                       NAME_COLUMN, name.c_str(),
                       ICON_COLUMN, "folder",
                       FLAGS_COLUMN, FLAG_IS_DIRECTORY,
                       -1);
    GtkTreeIter projIter(it);
    gtk_tree_store_append(m_store, &it, &projIter);
    gtk_tree_store_set(m_store, &it,
                       NAME_COLUMN, _("(Empty)"),
                       FLAGS_COLUMN, FLAG_IS_DUMMY,
                       -1);

    Connections conns;
    conns.closed = project.addClosedCallback(boost::bind(onProjectClosed,
                                                         this,
                                                         _1));
    conns.fileAdded =
        project.addFileAddedCallback(boost::bind(onProjectFileAdded,
                                                 this,
                                                 _1,
                                                 _2,
                                                 _3));
    conns.fileRemoved =
        project.addFileRemovedCallback(boost::bind(onProjectFileRemoved,
                                                   this,
                                                   _1,
                                                   _2));
    m_projConnsTable[project.uri()] = conns;
}

void ProjectExplorerModel::onProjectClosed(Project &project)
{
    char *projFileName = g_filename_from_uri(project.uri(), NULL, NULL);
    char *projBaseName = g_path_get_basename(projFileName);
    std::string name = projBaseName;
    name += " (";
    name += project.uri();
    name += ")";
    g_free(projFileName);
    g_free(projBaseName);

    GtkTreeIter it;
    bool added = false;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(m_store), &it))
    {
        do
        {
            char *nm;
            gtk_tree_model_get(GTK_TREE_MODEL(m_store), &it,
                               NAME_COLUMN, &nm, -1);
            int cmp = strcmp(nm, name.c_str());
            g_free(nm);
            if (cmp == 0)
            {
                gtk_tree_store_remove(m_store, &it);
                break;
            }
        }
        while (gtk_tree_model_iter_next(GTK_TREE_MODEL(m_store), &it));
    }

    ProjectConnectionsTable::iterator it2;
    it2 = m_projConnsTable.find(project.uri());
    it2->second.closed.disconnect();
    it2->second.fileAdded.disconnect();
    it2->second.fileRemoved.disconnect();
    m_projConnsTable.erase(it2);
}

void ProjectExplorerModel::onProjectFileAdded(Project &project,
                                              const char *uri,
                                              const ProjectFile &data)
{
}

void ProjectExplorerModel::onProjectFileRemoved(Project &project,
                                                const char *uri)
{
}

void ProjectExplorerModel::onRowExpanded(GtkTreeIter *iter)
{
}

}
