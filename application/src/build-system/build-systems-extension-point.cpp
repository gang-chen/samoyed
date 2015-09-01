// Extension point: build systems.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "build-systems-extension-point.hpp"
#include "build-system-extension.hpp"
#include "plugin/extension-point-manager.hpp"
#include "plugin/plugin-manager.hpp"
#include "application.hpp"
#include <assert.h>
#include <string.h>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <glib/gi18n.h>
#include <libxml/tree.h>

#define BUILD_SYSTEMS "build-systems"
#define DESCRIPTION "description"

namespace Samoyed
{

BuildSystemsExtensionPoint::BuildSystemsExtensionPoint():
    ExtensionPoint(BUILD_SYSTEMS)
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);
}

BuildSystemsExtensionPoint::~BuildSystemsExtensionPoint()
{
    Application::instance().extensionPointManager().
        unregisterExtensionPoint(*this);
    assert(m_extensions.empty());
}

bool BuildSystemsExtensionPoint::registerExtension(const char *extensionId,
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
                   DESCRIPTION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->description = value;
                xmlFree(value);
            }
        }
    }
    if (ext->description.empty())
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: Incomplete build system extension "
                  "specification.\n"),
                xmlNode->line);
            errors->push_back(cp);
            g_free(cp);
        }
        delete ext;
        return false;
    }
    m_extensions.insert(std::make_pair(ext->id.c_str(), ext));

    return true;
}

void BuildSystemsExtensionPoint::unregisterExtension(const char *extensionId)
{
    ExtensionInfo *ext = m_extensions[extensionId];
    m_extensions.erase(extensionId);
    delete ext;
}

BuildSystem *
BuildSystemsExtensionPoint::activateBuildSystem(Project &project,
                                                const char *extensionId)
{
    BuildSystemExtension *ext =
        static_cast<BuildSystemExtension *>(
            Application::instance().pluginManager().
            acquireExtension(extensionId));
    if (!ext)
        return NULL;
    BuildSystem *buildSystem = ext->activateBuildSystem(project);
    ext->release();
    return buildSystem;
}

}
