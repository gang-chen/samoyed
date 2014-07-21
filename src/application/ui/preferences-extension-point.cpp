// Extension point: preferences.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "preferences-extension-point.hpp"
#include "preferences-extension.hpp"
#include "application.hpp"
#include "utilities/extension-point-manager.hpp"
#include "utilities/plugin-manager.hpp"
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define PREFERENCES "preferences"
#define CATEGORY "category"

namespace Samoyed
{

PreferencesExtensionPoint::PreferencesExtensionPoint():
    ExtensionPoint(PREFERENCES)
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);
}

PreferencesExtensionPoint::~PreferencesExtensionPoint()
{
    for (ExtensionTable::iterator it = m_extensions.begin();
         it != m_extensions.end();)
    {
        ExtensionTable::iterator it2 = it;
        ++it;
        ExtensionInfo *ext = it2->second;
        m_extensions.erase(it2);
        delete ext;
    }

    Application::instance().extensionPointManager().
        unregisterExtensionPoint(*this);
}

bool PreferencesExtensionPoint::registerExtension(
    const char *extensionId,
    xmlNodePtr xmlNode,
    std::list<std::string> &errors)
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
                extInfo->categories.insert(value);
                xmlFree(value);
            }
        }
    }
    if (extInfo->categories.empty())
    {
        cp = g_strdup_printf(
            _("Line %d: Incomplete preferences extension specification.\n"),
            xmlNode->line);
        errors.push_back(cp);
        g_free(cp);
        delete extInfo;
        return false;
    }
    m_extensions.insert(std::make_pair(extInfo->id.c_str(), extInfo));

    // Install preferences.
    PreferencesExtension *ext = static_cast<PreferencesExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extensionId));
    if (!ext)
    {
        unregisterExtension(extensionId);
        return false;
    }
    ext->installPreferences();
    ext->release();
    return true;
}

void
PreferencesExtensionPoint::unregisterExtension(const char *extensionId)
{
    ExtensionInfo *ext = m_extensions[extensionId];
    m_extensions.erase(extensionId);
    delete ext;
}

void PreferencesExtensionPoint::categories(std::set<std::string> &categories)
{
    for (ExtensionTable::iterator it = m_extensions.begin();
         it != m_extensions.end();
         ++it)
        categories.insert(it->second->categories.begin(),
                          it->second->categories.end());
}

void
PreferencesExtensionPoint::setupPreferencesEditor(const char *category,
                                                  GtkWidget *grid)
{
    for (ExtensionTable::iterator it = m_extensions.begin();
         it != m_extensions.end();
         ++it)
    {
        ExtensionInfo &extInfo = *it->second;
        if (extInfo.categories.find(category) != extInfo.categories.end())
        {
            PreferencesExtension *ext = static_cast<PreferencesExtension *>(
                Application::instance().pluginManager().
                acquireExtension(extInfo.id.c_str()));
            if (ext)
            {
                ext->setupPreferencesEditor(category, grid);
                ext->release();
            }
        }
    }
}

}
