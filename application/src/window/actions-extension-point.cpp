// Extension point: actions.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "actions-extension-point.hpp"
#include "action-extension.hpp"
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

#define ACTIONS "actions"
#define TOGGLE "toggle"
#define ALWAYS_SENSITIVE "always-sensitive"
#define ACTIVE_BY_DEFAULT "active-by-default"
#define NAME "name"
#define PATH "path"
#define PATH_2 "path-2"
#define LABEL "label"
#define TOOLTIP "tooltip"
#define ICON_NAME "icon-name"
#define ACCELERATOR "accelerator"
#define SEPARATE "separate"

namespace Samoyed
{

void ActionsExtensionPoint::activateAction(const ExtensionInfo &extInfo,
                                           Window &window,
                                           GtkAction *action)
{
    ActionExtension *ext = static_cast<ActionExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extInfo.id.c_str()));
    if (!ext)
        return;
    ext->activateAction(window, action);
    ext->release();
}

void ActionsExtensionPoint::onActionToggled(const ExtensionInfo &extInfo,
                                            Window &window,
                                            GtkToggleAction *action)
{
    ActionExtension *ext = static_cast<ActionExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extInfo.id.c_str()));
    if (!ext)
        return;
    ext->onActionToggled(window, action);
    ext->release();
}

bool ActionsExtensionPoint::isActionSensitive(const ExtensionInfo &extInfo,
                                              Window &window,
                                              GtkAction *action)
{
    if (extInfo.alwaysSensitive)
        return true;
    ActionExtension *ext = static_cast<ActionExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extInfo.id.c_str()));
    if (!ext)
        return true;
    bool sensitive = ext->isActionSensitive(window, action);
    ext->release();
    return sensitive;
}

void
ActionsExtensionPoint::registerExtensionInternally(Window &window,
                                                   const ExtensionInfo &extInfo)
{
    if (extInfo.toggle)
    {
        window.addToggleAction(extInfo.name.c_str(),
                               extInfo.path.c_str(),
                               extInfo.path2.empty() ?
                               NULL : extInfo.path2.c_str(),
                               extInfo.label.c_str(),
                               extInfo.tooltip.c_str(),
                               extInfo.iconName.empty() ?
                               NULL : extInfo.iconName.c_str(),
                               extInfo.accelerator.c_str(),
                               boost::bind(onActionToggled,
                                           boost::cref(extInfo), _1, _2),
                               boost::bind(isActionSensitive,
                                           boost::cref(extInfo), _1, _2),
                               extInfo.activeByDefault,
                               extInfo.separate);
    }
    else
    {
        window.addAction(extInfo.name.c_str(),
                         extInfo.path.c_str(),
                         extInfo.path2.empty() ?
                         NULL : extInfo.path2.c_str(),
                         extInfo.label.c_str(),
                         extInfo.tooltip.c_str(),
                         extInfo.iconName.empty() ?
                         NULL : extInfo.iconName.c_str(),
                         extInfo.accelerator.c_str(),
                         boost::bind(activateAction,
                                     boost::cref(extInfo), _1, _2),
                         boost::bind(isActionSensitive,
                                     boost::cref(extInfo), _1, _2),
                         extInfo.separate);
    }
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

    Application::instance().extensionPointManager().
        unregisterExtensionPoint(*this);

    assert(m_extensions.empty());
}

void ActionsExtensionPoint::registerAllExtensions(Window &window)
{
    for (ExtensionTable::const_iterator it = m_extensions.begin();
         it != m_extensions.end();
         ++it)
        registerExtensionInternally(window, *it->second);
}

bool ActionsExtensionPoint::registerExtension(const char *extensionId,
                                              xmlNodePtr xmlNode,
                                              std::list<std::string> *errors)
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
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                        child->line, value, TOGGLE, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ALWAYS_SENSITIVE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    ext->alwaysSensitive = boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                        child->line, value, ALWAYS_SENSITIVE, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
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
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                        child->line, value, ACTIVE_BY_DEFAULT, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        NAME) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->name = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        PATH) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->path = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        PATH_2) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->path2 = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        LABEL) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {   
                ext->label = gettext(value);
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        TOOLTIP) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->tooltip = gettext(value);
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ICON_NAME) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->iconName = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ACCELERATOR) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->accelerator = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        SEPARATE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    ext->separate = boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                        child->line, value, SEPARATE, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
            }
        }
    }
    if (ext->name.empty() ||
        ext->path.empty() ||
        ext->label.empty())
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: Incomplete action extension specification.\n"),
                xmlNode->line);
            errors->push_back(cp);
            g_free(cp);
        }
        delete ext;
        return false;
    }
    m_extensions.insert(std::make_pair(ext->id.c_str(), ext));

    // Register the action to each existing window.
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
        registerExtensionInternally(*window, *ext);

    return true;
}

void ActionsExtensionPoint::unregisterExtension(const char *extensionId)
{
    ExtensionInfo *ext = m_extensions[extensionId];
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
        window->removeAction(ext->name.c_str());
    m_extensions.erase(extensionId);
    delete ext;
}

}
