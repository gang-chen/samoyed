// Project explorer.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-explorer.hpp"
#include "project.hpp"
#include "project-file.hpp"
#include "window/window.hpp"
#include "application.hpp"
#include <string.h>
#include <list>
#include <string>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define WIDGET "widget"
#define PROJECT_EXPLORER "project-explorer"
#define NAVIGATION_PANE_ID "navigation-pane"
#define PROJECT_EXPLORER_ID "project-explorer"

namespace
{

const int PROJECT_EXPLORER_INDEX_IN_NAVIGATION_PANE = 1;

enum Column
{
    COLUMN_NAME,
    COLUMN_TYPE,
    COLUMN_URI,
    COLUMN_ICON,
    N_COLUMNS
};

}

namespace Samoyed
{

void ProjectExplorer::XmlElement::registerReader()
{
    Widget::XmlElement::registerReader(PROJECT_EXPLORER,
                                       Widget::XmlElement::Reader(read));
}

bool ProjectExplorer::XmlElement::readInternally(xmlNodePtr node,
                                                 std::list<std::string> *errors)
{
    char *value, *cp;
    bool widgetSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   WIDGET) == 0)
        {
            if (widgetSeen)
            {
                if (errors)
                {
                    cp = g_strdup_printf(
                        _("Line %d: More than one \"%s\" elements seen.\n"),
                        child->line, WIDGET);
                    errors->push_back(cp);
                    g_free(cp);
                }
                return false;
            }
            if (!Widget::XmlElement::readInternally(child, errors))
                return false;
            widgetSeen = true;
        }
    }

    if (!widgetSeen)
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: \"%s\" element missing.\n"),
                node->line, WIDGET);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }

    return true;
}

ProjectExplorer::XmlElement *
ProjectExplorer::XmlElement::read(xmlNodePtr node,
                                  std::list<std::string> *errors)
{
    XmlElement *element = new XmlElement;
    if (!element->readInternally(node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr ProjectExplorer::XmlElement::write() const
{
    xmlNodePtr node =
        xmlNewNode(NULL,
                   reinterpret_cast<const xmlChar *>(PROJECT_EXPLORER));
    xmlAddChild(node, Widget::XmlElement::write());
    return node;
}

ProjectExplorer::XmlElement::XmlElement(const ProjectExplorer &explorer):
    Widget::XmlElement(explorer)
{
}

Widget *ProjectExplorer::XmlElement::restoreWidget()
{
    ProjectExplorer *explorer = new ProjectExplorer;
    if (!explorer->restore(*this))
    {
        delete explorer;
        return NULL;
    }
    return explorer;
}

bool ProjectExplorer::setupProjectExplorer()
{
    GtkTreeStore *store = gtk_tree_store_new(N_COLUMNS,
                                             G_TYPE_STRING,
                                             G_TYPE_UINT,
                                             G_TYPE_STRING,
                                             GDK_TYPE_PIXBUF);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    GtkTreeViewColumn *column;
    GtkCellRenderer *renderer;
    column = gtk_tree_view_column_new();
    renderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_add_attribute(column, renderer,
                                       "pixbuf", COLUMN_ICON);
    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(column, renderer, TRUE);
    gtk_tree_view_column_add_attribute(column, renderer,
                                       "markup", COLUMN_NAME);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
    setGtkWidget(tree);

    // Load the existing projects.
    for (Project *project = Application::instance().projects();
         project;
         project = project->next())
        onProjectOpened(*project);

    return true;
}

bool ProjectExplorer::setup()
{
    if (!Widget::setup(PROJECT_EXPLORER_ID))
        return false;
    setTitle(_("_Project Explorer"));
    if (!setupProjectExplorer())
        return false;
    gtk_widget_show_all(gtkWidget());
    return true;
}

bool ProjectExplorer::restore(XmlElement &xmlElement)
{
    if (!Widget::restore(xmlElement))
        return false;
    if (!setupProjectExplorer())
        return false;
    if (xmlElement.visible())
        gtk_widget_show_all(gtkWidget());
    return true;
}

ProjectExplorer *ProjectExplorer::create()
{
    ProjectExplorer *explorer = new ProjectExplorer;
    if (!explorer->setup())
    {
        delete explorer;
        return NULL;
    }
    return explorer;
}

Widget::XmlElement *ProjectExplorer::save() const
{
    return new XmlElement(*this);
}

void ProjectExplorer::onWindowCreated(Window &window)
{
    window.registerSidePaneChild(NAVIGATION_PANE_ID, PROJECT_EXPLORER_ID,
                                 PROJECT_EXPLORER_INDEX_IN_NAVIGATION_PANE,
                                 create, _("_Project Explorer"));
    window.openSidePaneChild(NAVIGATION_PANE_ID, PROJECT_EXPLORER_ID);
}

void ProjectExplorer::onWindowRestored(Window &window)
{
    window.registerSidePaneChild(NAVIGATION_PANE_ID, PROJECT_EXPLORER_ID,
                                 PROJECT_EXPLORER_INDEX_IN_NAVIGATION_PANE,
                                 create, _("_Project Explorer"));
}

void ProjectExplorer::registerWithWindow()
{
    Window::addCreatedCallback(onWindowCreated);
    Window::addRestoredCallback(onWindowRestored);
}

Project *ProjectExplorer::currentProject()
{
    return Application::instance().projects();
}

const Project *ProjectExplorer::currentProject() const
{
    return Application::instance().projects();
}

void ProjectExplorer::onProjectOpened(Project &project)
{
}

void ProjectExplorer::onProjectClosed(Project &project)
{
}

void ProjectExplorer::onProjectFileAdded(Project &project,
                                         const char *uri,
                                         const ProjectFile &data)
{
}

void ProjectExplorer::onProjectFileRemoved(Project &project,
                                           const char *uri)
{
}

}
