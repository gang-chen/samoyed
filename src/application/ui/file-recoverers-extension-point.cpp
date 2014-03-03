// Extension point: file recoverers.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-recoverers-extension-point.hpp"
#include "file-recoverer-extension.hpp"
#include "file.hpp"
#include "application.hpp"
#include "utilities/extension-point-manager.hpp"
#include "utilities/plugin-manager.hpp"
#include <string.h>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <libxml/tree.h>

#define FILE_RECOVERERS "file-recoverers"
#define TYPE "type"

namespace Samoyed
{

FileRecoverersExtensionPoint::FileRecoverersExtensionPoint():
    ExtensionPoint(FILE_RECOVERERS)
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);
}

FileRecoverersExtensionPoint::~FileRecoverersExtensionPoint()
{
    Application::instance().extensionPointManager().
        unregisterExtensionPoint(*this);
}

bool FileRecoverersExtensionPoint::registerExtension(const char *extensionId,
                                                     xmlNodePtr xmlNode,
                                                     std::list<std::string> &errors)
{
    ExtensionInfo *ext = new ExtensionInfo(extensionId);
    char *value, *cp;
    for (xmlNodePtr child = xmlNode->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   TYPE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                ext->type = value;
                xmlFree(value);
            }
        }
    }
    if (ext->type.empty())
    {
        cp = g_strdup_printf(
            _("Line %d: Incomplete file recoverer extension specification.\n"),
            xmlNode->line);
        errors.push_back(cp);
        g_free(cp);
        delete ext;
        return false;
    }
    m_extensions.insert(std::make_pair(ext->id.c_str(), ext));
    return true;
}

void FileRecoverersExtensionPoint::unregisterExtension(const char *extensionId)
{
    ExtensionInfo *ext = m_extensions[extensionId];
    m_extensions.erase(extensionId);
    delete ext;
}

void FileRecoverersExtensionPoint::recoverFile(const char *fileUri)
{
    char *fileName = g_filename_from_uri(fileUri, NULL, NULL);
    char *type = g_content_type_guess(fileName, NULL, 0, NULL);
    File *file;
    for (ExtensionTable::const_iterator it = m_extensions.begin();
         it != m_extensions.end();
         ++it)
    {
        ExtensionInfo *extInfo = it->second;
        if (g_content_type_is_a(type, extInfo->type.c_str()))
        {
            FileRecovererExtension *ext =
                static_cast<FileRecovererExtension *>(
                    Application::instance().pluginManager().
                    acquireExtension(extInfo->id.c_str()));
            if (ext)
            {
                file = File::open(fileUri, NULL, options, false);
                ext->recoverFile(file);
                ext->release();
                break;
            }
        }
    }
    g_free(fileName);
    g_free(type);
}

}
