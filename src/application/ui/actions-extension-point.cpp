// Extension point: actions.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "actions-extension-point.hpp"
#include "action-extension.hpp"
#include "window.hpp"
#include "application.hpp"
#include "utilities/extension-point-manager.hpp"
#include "utilities/plugin-manager.hpp"
#include <string.h>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <libxml/tree.h>

#define ACTIONS "actions"
#define TOGGLE "toggle"
#define ACTIVE_BY_DEFAULT "active-by-default"
#define ACTION_NAME "action-name"
#define ACTION_PATH "action-path"
#define MENU_TITLE "menu-title"
#define MENU_TOOLTIP "menu-tooltip"

namespace Samoyed
{

void ActionsExtensionPoint::activateAction(const ExtensionInfo &extInfo,
                                           Window &window)
{
    ActionExtension *ext = static_cast<ActionExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extInfo.id.c_str()));
    if (!ext)
        return;
    ext->activateAction(window);
    ext->release();
}

void ActionsExtensionPoint::onActionToggled(const ExtensionInfo &extInfo,
                                            Window &window,
                                            bool active)
{
    ActionExtension *ext = static_cast<ActionExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extInfo.id.c_str()));
    if (!ext)
        return;
    ext->onActionToggled(window, active);
    ext->release();
}

ActionsExtensionPoint::ActionsExtensionPoint():
    ExtensionPoint(ACTIONS)
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);

    m_windowsCreatedConnection = Window::addCreatedCallback(boost::bind(
        &ActionsExtensionPoint::registerAllExtensions, this, _1));
    m_windowsRestoredConnection = Window::addRestoredCallback(boost::bind(
        &ActionsExtensionPoint::registerAllExtensions, this, _1));
}

ActionsExtensionPoint::~ActionsExtensionPoint()
{
    m_windowsCreatedConnection.disconnect();
    m_windowsRestoredConnection.disconnect();

    for (ExtensionTable::iterator it = m_extensions.begin();
         it != m_extensions.end();)
    {
        ExtensionTable::iterator it2 = it;
        ++it;
        unregisterExtension(it2->first);
    }

    Application::instance().extensionPointManager().
        unregisterExtensionPoint(*this);
}

void ActionsExtensionPoint::registerAllExtensions(Window &window)
{
    for (ExtensionTable::const_iterator it = m_extensions.begin();
         it != m_extensions.end();
         ++it)
    {
        if (it->second->toggle)
            window.addToggleAction(it->second->actionName.c_str(),
                                   it->second->actionPath.c_str(),
                                   it->second->menuTitle.c_str(),
                                   it->second->menuTooltip.c_str(),
                                   boost::bind(onActionToggled,
                                               boost::cref(*it->second),
                                               _1, _2),
                                   it->second->activeByDefault);
        else
            window.addAction(it->second->actionName.c_str(),
                             it->second->actionPath.c_str(),
                             it->second->menuTitle.c_str(),
                             it->second->menuTooltip.c_str(),
                             boost::bind(activateAction,
                                         boost::cref(*it->second), _1));
    }
}

bool ActionsExtensionPoint::registerExtension(const char *extensionId,
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
                   TOGGLE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    ext->toggle = boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid Boolean value \"%s\" for element "
                          "\"%s\". %s.\n"),
                    child->line, value, TOGGLE, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ACTIVE_BY_DEFAULT) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    ext->activeByDefault = boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &exp)
                {
                    cp = g_strdup_printf(
                        _("Line %d: Invalid Boolean value \"%s\" for element "
                          "\"%s\". %s.\n"),
                    child->line, value, ACTIVE_BY_DEFAULT, exp.what());
                    errors.push_back(cp);
                    g_free(cp);
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ACTION_NAME) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->actionName = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ACTION_PATH) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->actionPath = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        MENU_TITLE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {   
                ext->menuTitle = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        MENU_TOOLTIP) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->menuTooltip = value;
                xmlFree(value);
            }
        }
    }
    if (ext->actionName.empty() ||
        ext->actionPath.empty() ||
        ext->menuTitle.empty())
    {
        cp = g_strdup_printf(
            _("Line %d: Incomplete action extension specification.\n"),
            xmlNode->line);
        errors.push_back(cp);
        g_free(cp);
        delete ext;
        return false;
    }
    m_extensions.insert(std::make_pair(ext->id.c_str(), ext));

    // Register the action to each existing window.
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
    {
        if (ext->toggle)
            window->addToggleAction(ext->actionName.c_str(),
                                    ext->actionPath.c_str(),
                                    ext->menuTitle.c_str(),
                                    ext->menuTooltip.c_str(),
                                    boost::bind(onActionToggled,
                                                boost::cref(*ext), _1, _2),
                                    ext->activeByDefault);
        else
            window->addAction(ext->actionName.c_str(),
                              ext->actionPath.c_str(),
                              ext->menuTitle.c_str(),
                              ext->menuTooltip.c_str(),
                              boost::bind(activateAction,
                                          boost::cref(*ext), _1));
    }

    return true;
}

void ActionsExtensionPoint::unregisterExtension(const char *extensionId)
{
    ExtensionInfo *ext = m_extensions[extensionId];
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
        window->removeAction(ext->actionName.c_str());
    m_extensions.erase(extensionId);
    delete ext;
}

}
