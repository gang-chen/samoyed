// Extension point: views.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "views-extension-point.hpp"
#include "view-extension.hpp"
#include "view.hpp"
#include "window.hpp"
#include "application.hpp"
#include "utilities/extension-point-manager.hpp"
#include "utilities/plugin-manager.hpp"
#include <list>
#include <string>
#include <utility>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/tree.h>

#define VIEWS "views"
#define CATEGORY "category"
#define NAVIGATOR "navigator"
#define TOOL "tool"
#define INDEX "index"
#define OPEN_BY_DEFAULT "open-by-default"
#define VIEW "view"
#define ID "id"
#define TITLE "title"
#define MENU_ITEM "menu-item"

namespace
{

Samoyed::Widget *
createView(const Samoyed::ViewsExtensionPoint::ExtensionInfo &extInfo)
{
    Samoyed::ViewExtension *ext =
        static_cast<Samoyed::ViewExtension *>(Samoyed::Application::instance().
                pluginManager().acquireExtension(extInfo.id.c_str()));
    if (!ext)
        return NULL;
    Samoyed::Widget *widget = ext->createView(extInfo.viewId.c_str(),
                                              extInfo.viewTitle.c_str());
    ext->release();
    return widget;
}

void registerExtensionInternally(
    Samoyed::Window &window,
    const Samoyed::ViewsExtensionPoint::ExtensionInfo &ext)
{
    window.registerSidePaneChild(ext.paneId.c_str(),
                                 ext.viewId.c_str(),
                                 ext.viewIndex,
                                 boost::bind(createView, boost::cref(ext)),
                                 ext.menuTitle.c_str());
    if (ext.openByDefault)
        window.openSidePaneChild(ext.paneId.c_str(), ext.viewId.c_str());
}

}

namespace Samoyed
{

ViewsExtensionPoint::ViewsExtensionPoint():
    ExtensionPoint(VIEWS)
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);

    m_createdConnection = Window::addCreatedCallback(boost::bind(
        &ViewsExtensionPoint::registerAllExtensions, this, _1));
    m_restoredConnection = Window::addRestoredCallback(boost::bind(
        &ViewsExtensionPoint::registerAllExtensions, this, _1));
}

ViewsExtensionPoint::~ViewsExtensionPoint()
{
    for (ExtensionTable::iterator it = m_extensions.begin();
         it != m_extensions.end();)
    {
        ExtensionTable::iterator it2 = it;
        ++it;
        unregisterExtension(it2->first);
    }

    m_createdConnection.disconnect();
    m_restoredConnection.disconnect();

    Application::instance().extensionPointManager().
        unregisterExtensionPoint(*this);
}

void ViewsExtensionPoint::registerAllExtensions(Window &window)
{
    for (ExtensionTable::const_iterator it = m_extensions.begin();
         it != m_extensions.end();
         ++it)
        registerExtensionInternally(window, *it->second);
}

bool ViewsExtensionPoint::registerExtension(const char *extensionId,
                                            xmlNodePtr xmlNode,
                                            std::list<std::string> &errors)
{
    // Parse the extension.
    ExtensionInfo *ext = new ExtensionInfo(extensionId);
    char *value, *cp;
    for (xmlNodePtr child = xmlNode->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   CATEGORY) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                if (strcmp(value, NAVIGATOR) == 0)
                    ext->paneId = Window::NAVIGATION_PANE_ID;
                else if (strcmp(value, TOOL) == 0)
                    ext->paneId = Window::TOOLS_PANE_ID;
                else
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid view category \"%s\".\n"),
                        child->line, value);
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        INDEX) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    ext->viewIndex = boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid integer \"%s\" for element \"%s\". "
                          "%s.\n"),
                        child->line, value, INDEX, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        OPEN_BY_DEFAULT) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    ext->openByDefault = boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid Boolean value \"%s\" for element "
                          "\"%s\". %s.\n"),
                        child->line, value, OPEN_BY_DEFAULT, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        VIEW) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                if (grandChild->type != XML_ELEMENT_NODE)
                    continue;
                if (strcmp(reinterpret_cast<const char *>(grandChild->name),
                           ID) == 0)
                {
                    value = reinterpret_cast<char *>(
                        xmlNodeGetContent(grandChild->children));
                    if (value)
                    {
                        ext->viewId = value;
                        xmlFree(value);
                    }
                }
                else if (strcmp(reinterpret_cast<const char *>(
                                    grandChild->name),
                                TITLE) == 0)
                {
                    value = reinterpret_cast<char *>(
                        xmlNodeGetContent(grandChild->children));
                    if (value)
                    {
                        ext->viewTitle = value;
                        xmlFree(value);
                    }
                }
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        MENU_ITEM) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                if (grandChild->type != XML_ELEMENT_NODE)
                    continue;
                if (strcmp(reinterpret_cast<const char *>(grandChild->name),
                           TITLE) == 0)
                {
                    value = reinterpret_cast<char *>(
                        xmlNodeGetContent(grandChild->children));
                    if (value)
                    {
                        ext->menuTitle = value;
                        xmlFree(value);
                    }
                }
            }
        }
    }

    if (ext->menuTitle.empty())
        ext->menuTitle = ext->viewTitle;
    if (ext->paneId.empty() || ext->viewId.empty() || ext->viewTitle.empty())
    {
        cp = g_strdup_printf(
            _("Line %d: Incomplete view extension specification.\n"),
            xmlNode->line);
        errors.push_back(cp);
        g_free(cp);
        delete ext;
        return false;
    }
    m_extensions.insert(std::make_pair(ext->id.c_str(), ext));

    // Register the view to each existing window.
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
        registerExtensionInternally(*window, *ext);

    return true;
}

void ViewsExtensionPoint::unregisterExtension(const char *extensionId)
{
    ExtensionInfo *ext = m_extensions[extensionId];
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
        window->unregisterSidePaneChild(ext->paneId.c_str(),
                                        ext->viewId.c_str());
    m_extensions.erase(extensionId);
    delete ext;
}

}
