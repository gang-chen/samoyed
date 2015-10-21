// Build log view group.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "build-log-view-group.hpp"
#include "build-log-view.hpp"
#include "window/window.hpp"
#include <string.h>
#include <list>
#include <string>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define NOTEBOOK "notebook"
#define BUILD_LOG_VIEW_GROUP "build-log-view-group"
#define TOOLS_PANE_ID "tools-pane"
#define BUILD_LOG_VIEW_GROUP_ID "build-log-view-group"

namespace
{

const int BUILD_LOG_VIEW_GROUP_INDEX_IN_TOOLS_PANE = 1;

}

namespace Samoyed
{

void BuildLogViewGroup::XmlElement::registerReader()
{
    Widget::XmlElement::registerReader(BUILD_LOG_VIEW_GROUP,
                                       Widget::XmlElement::Reader(read));
}

bool
BuildLogViewGroup::XmlElement::readInternally(xmlNodePtr node,
                                              std::list<std::string> *errors)
{
    char *value, *cp;
    bool notebookSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   NOTEBOOK) == 0)
        {
            if (notebookSeen)
            {
                if (errors)
                {
                    cp = g_strdup_printf(
                        _("Line %d: More than one \"%s\" elements seen.\n"),
                        child->line, NOTEBOOK);
                    errors->push_back(cp);
                    g_free(cp);
                }
                return false;
            }
            if (!Notebook::XmlElement::readInternally(child, errors))
                return false;
            notebookSeen = true;
        }
    }

    if (!notebookSeen)
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: \"%s\" element missing.\n"),
                node->line, NOTEBOOK);
            errors->push_back(cp);
            g_free(cp);
        }
        return false;
    }

    return true;
}

BuildLogViewGroup::XmlElement *
BuildLogViewGroup::XmlElement::read(xmlNodePtr node,
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

xmlNodePtr BuildLogViewGroup::XmlElement::write() const
{
    xmlNodePtr node =
        xmlNewNode(NULL,
                   reinterpret_cast<const xmlChar *>(BUILD_LOG_VIEW_GROUP));
    xmlAddChild(node, Notebook::XmlElement::write());
    return node;
}

BuildLogViewGroup::XmlElement::XmlElement(const BuildLogViewGroup &group):
    Notebook::XmlElement(group)
{
}

Widget *BuildLogViewGroup::XmlElement::restoreWidget()
{
    BuildLogViewGroup *group = new BuildLogViewGroup;
    if (!group->restore(*this))
    {
        delete group;
        return NULL;
    }
    return group;
}

bool BuildLogViewGroup::setup()
{
    if (!Notebook::setup(BUILD_LOG_VIEW_GROUP_ID, NULL, true, false, true))
        return false;
    setTitle(_("_Build Log Viewer"));
    return true;
}

BuildLogViewGroup *BuildLogViewGroup::create()
{
    BuildLogViewGroup *group = new BuildLogViewGroup;
    if (!group->setup())
    {
        delete group;
        return NULL;
    }
    return group;
}

Widget::XmlElement *BuildLogViewGroup::save() const
{
    return new XmlElement(*this);
}

void BuildLogViewGroup::onWindowCreatedOrRestored(Window &window)
{
    window.registerSidePaneChild(TOOLS_PANE_ID, BUILD_LOG_VIEW_GROUP_ID,
                                 BUILD_LOG_VIEW_GROUP_INDEX_IN_TOOLS_PANE,
                                 create, _("_Build Log Viewer"));
}

void BuildLogViewGroup::registerWithWindow()
{
    Window::addCreatedCallback(onWindowCreatedOrRestored);
    Window::addRestoredCallback(onWindowCreatedOrRestored);
}

void BuildLogViewGroup::buildLogViewsForProject(
    const char *projectUri,
    std::list<BuildLogView *> &views)
{
    for (int i = 0; i < childCount(); i++)
    {
        BuildLogView &view = static_cast<BuildLogView &>(child(i));
        if (strcmp(view.projectUri(), projectUri) == 0)
            views.push_back(&view);
    }
}

void BuildLogViewGroup::buildLogViewsForProject(
    const char *projectUri,
    std::list<const BuildLogView *> &views) const
{
    for (int i = 0; i < childCount(); i++)
    {
        const BuildLogView &view = static_cast<const BuildLogView &>(child(i));
        if (strcmp(view.projectUri(), projectUri) == 0)
            views.push_back(&view);
    }
}

BuildLogView *
BuildLogViewGroup::buildLogViewForConfiguration(const char *projectUri,
                                                const char *configName)
{
    for (int i = 0; i < childCount(); i++)
    {
        BuildLogView &view = static_cast<BuildLogView &>(child(i));
        if (strcmp(view.projectUri(), projectUri) == 0 &&
            strcmp(view.configurationName(), configName) == 0)
            return &view;
    }
    return NULL;
}

const BuildLogView *
BuildLogViewGroup::buildLogViewForConfiguration(const char *projectUri,
                                                const char *configName) const
{
    for (int i = 0; i < childCount(); i++)
    {
        const BuildLogView &view = static_cast<const BuildLogView &>(child(i));
        if (strcmp(view.projectUri(), projectUri) == 0 &&
            strcmp(view.configurationName(), configName) == 0)
            return &view;
    }
    return NULL;
}

BuildLogView *BuildLogViewGroup::openBuildLogView(const char *projectUri,
                                                  const char *configName)
{
    BuildLogView *view = buildLogViewForConfiguration(projectUri, configName);
    if (!view)
    {
        view = BuildLogView::create(projectUri, configName);
        int index = childCount();
        addChild(*view, index);
    }
    view->setCurrent();
    return view;
}

}
