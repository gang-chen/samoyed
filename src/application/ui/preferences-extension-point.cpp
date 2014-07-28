// Extension point: preferences.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "preferences-extension-point.hpp"
#include "preferences-extension.hpp"
#include "preferences-editor.hpp"
#include "application.hpp"
#include "utilities/extension-point-manager.hpp"
#include "utilities/plugin-manager.hpp"
#include <assert.h>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define PREFERENCES "preferences"
#define CATEGORY "category"
#define ID "id"
#define LABEL "label"

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
    Application::instance().extensionPointManager().
        unregisterExtensionPoint(*this);

    assert(m_extensions.empty());
}

bool PreferencesExtensionPoint::registerExtension(
    const char *extensionId,
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
            std::string id, label;
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
                        id = value;
                        xmlFree(value);
                    }
                }
                else if (strcmp(reinterpret_cast<const char *>
                                    (grandChild->name),
                                LABEL) == 0)
                {
                    value = reinterpret_cast<char *>(
                        xmlNodeGetContent(grandChild->children));
                    if (value)
                    {
                        label = value;
                        xmlFree(value);
                    }
                }
            }
            if (!id.empty())
                extInfo->categories[id] = label;
        }
    }
    if (extInfo->categories.empty())
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: Incomplete preferences extension specification.\n"),
                xmlNode->line);
            errors->push_back(cp);
            g_free(cp);
        }
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

    for (std::map<std::string, std::string>::iterator it =
         extInfo->categories.begin();
         it != extInfo->categories.end();
         ++it)
        if (!it->second.empty())
            PreferencesEditor::addCategory(it->first.c_str(),
                                           it->second.c_str());

    return true;
}

void
PreferencesExtensionPoint::unregisterExtension(const char *extensionId)
{
    ExtensionInfo *extInfo = m_extensions[extensionId];
    PreferencesExtension *ext = static_cast<PreferencesExtension *>(
        Application::instance().pluginManager().
        acquireExtension(extensionId));
    if (ext)
    {
        ext->uninstallPreferences();
        ext->release();
    }
    for (std::map<std::string, std::string>::iterator it =
         extInfo->categories.begin();
         it != extInfo->categories.end();
         ++it)
        if (!it->second.empty())
            PreferencesEditor::removeCategory(it->first.c_str());
    m_extensions.erase(extensionId);
    delete extInfo;
}

void
PreferencesExtensionPoint::setupPreferencesEditor(const char *category,
                                                  GtkGrid *grid)
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
