// Extension point: views.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "views-extension-point.hpp"
#include "widget/view-extension.hpp"
#include "widget/view.hpp"
#include "window.hpp"
#include "plugin/extension-point-manager.hpp"
#include "plugin/plugin-manager.hpp"
#include "application.hpp"
#include <assert.h>
#include <string.h>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <libintl.h>
#include <glib.h>
#include <glib/gi18n.h>
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
#define LABEL "label"

namespace Samoyed
{

Widget *ViewsExtensionPoint::createView(const ExtensionInfo &extInfo)
{
    ViewExtension *ext = static_cast<ViewExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extInfo.id.c_str()));
    if (!ext)
        return NULL;
    Widget *widget = ext->createView(extInfo.viewId.c_str(),
                                     extInfo.viewTitle.c_str());
    ext->release();
    return widget;
}

void ViewsExtensionPoint::registerExtensionInternally(Window &window,
                                                      const ExtensionInfo &ext)
{
    window.registerSidePaneChild(ext.paneId.c_str(),
                                 ext.viewId.c_str(),
                                 ext.viewIndex,
                                 boost::bind(createView, boost::cref(ext)),
                                 ext.menuItemLabel.c_str());
    if (ext.openByDefault)
        window.openSidePaneChild(ext.paneId.c_str(), ext.viewId.c_str());
}

ViewsExtensionPoint::ViewsExtensionPoint():
    ExtensionPoint(VIEWS)
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);

    m_windowsCreatedConnection = Window::addCreatedCallback(boost::bind(
        &ViewsExtensionPoint::registerAllExtensions, this, _1));
    m_windowsRestoredConnection = Window::addRestoredCallback(boost::bind(
        &ViewsExtensionPoint::registerAllExtensions, this, _1));
}

ViewsExtensionPoint::~ViewsExtensionPoint()
{
    m_windowsCreatedConnection.disconnect();
    m_windowsRestoredConnection.disconnect();

    Application::instance().extensionPointManager().
        unregisterExtensionPoint(*this);

    assert(m_extensions.empty());
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
                                            std::list<std::string> *errors)
{
    // Parse the extension.
    ExtensionInfo *extInfo = new ExtensionInfo(extensionId);
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
                    extInfo->paneId = Window::NAVIGATION_PANE_ID;
                else if (strcmp(value, TOOL) == 0)
                    extInfo->paneId = Window::TOOLS_PANE_ID;
                else if (errors)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid view category \"%s\".\n"),
                        child->line, value);
                    errors->push_back(cp);
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
                    extInfo->viewIndex = boost::lexical_cast<int>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid integer \"%s\" for element "
                              "\"%s\". %s.\n"),
                            child->line, value, INDEX, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                    extInfo->openByDefault = boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                            child->line, value, OPEN_BY_DEFAULT, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                        extInfo->viewId = value;
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
                        extInfo->viewTitle = gettext(value);
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
                           LABEL) == 0)
                {
                    value = reinterpret_cast<char *>(
                        xmlNodeGetContent(grandChild->children));
                    if (value)
                    {
                        extInfo->menuItemLabel = gettext(value);
                        xmlFree(value);
                    }
                }
            }
        }
    }

    if (extInfo->menuItemLabel.empty())
        extInfo->menuItemLabel = extInfo->viewTitle;
    if (extInfo->paneId.empty() ||
        extInfo->viewId.empty() ||
        extInfo->viewTitle.empty())
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: Incomplete view extension specification.\n"),
                xmlNode->line);
            errors->push_back(cp);
            g_free(cp);
        }
        delete extInfo;
        return false;
    }
    m_extensions.insert(std::make_pair(extInfo->id.c_str(), extInfo));

    // Register the XML element readers for the views.
    ViewExtension *ext = static_cast<ViewExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extensionId));
    if (!ext)
    {
        unregisterExtension(extensionId);
        return false;
    }
    ext->registerXmlElementReader();
    ext->release();

    // Register the view to each existing window.
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
        registerExtensionInternally(*window, *extInfo);

    return true;
}

void ViewsExtensionPoint::unregisterExtension(const char *extensionId)
{
    ViewExtension *ext = static_cast<ViewExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extensionId));
    if (ext)
    {
        ext->unregisterXmlElementReader();
        ext->release();
    }
    ExtensionInfo *extInfo = m_extensions[extensionId];
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
        window->unregisterSidePaneChild(extInfo->paneId.c_str(),
                                        extInfo->viewId.c_str());
    m_extensions.erase(extensionId);
    delete extInfo;
}

}
