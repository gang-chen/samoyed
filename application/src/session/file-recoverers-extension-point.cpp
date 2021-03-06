// Extension point: file recoverers.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-recoverers-extension-point.hpp"
#include "file-recoverer-extension.hpp"
#include "editors/file.hpp"
#include "editors/editor.hpp"
#include "plugin/extension-point-manager.hpp"
#include "plugin/plugin-manager.hpp"
#include "window/window.hpp"
#include "widget/notebook.hpp"
#include "application.hpp"
#include <assert.h>
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
#define MIME_TYPE "mime-type"

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

    assert(m_extensions.empty());
}

bool FileRecoverersExtensionPoint::registerExtension(const char *extensionId,
                                                     xmlNodePtr xmlNode,
                                                     std::list<std::string> *errors)
{
    ExtensionInfo *ext = new ExtensionInfo(extensionId);
    char *value, *cp;
    for (xmlNodePtr child = xmlNode->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   MIME_TYPE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                cp = g_content_type_from_mime_type(value);
                ext->type = cp;
                g_free(cp);
                xmlFree(value);
            }
        }
    }
    if (ext->type.empty())
    {
        if (errors)
        {
            cp = g_strdup_printf(
                _("Line %d: Incomplete file recoverer extension "
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

void FileRecoverersExtensionPoint::unregisterExtension(const char *extensionId)
{
    ExtensionInfo *ext = m_extensions[extensionId];
    m_extensions.erase(extensionId);
    delete ext;
}

void FileRecoverersExtensionPoint::recoverFile(const char *fileUri,
                                               long timeStamp,
                                               const char *fileMimeType,
                                               const PropertyTree &fileOptions)
{
    char *type = g_content_type_from_mime_type(fileMimeType);
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
                std::pair<File *, Editor *> fileEditor =
                    File::open(fileUri, NULL,
                               fileMimeType, &fileOptions, false);
                if (fileEditor.first)
                {
                    if (!fileEditor.second->parent())
                    {
                        Window &window =
                            *Application::instance().currentWindow();
                        Notebook &editorGroup = window.currentEditorGroup();
                        window.addEditorToEditorGroup(
                                *fileEditor.second,
                                editorGroup,
                                editorGroup.currentChildIndex() + 1);
                    }
                    ext->recoverFile(*fileEditor.first, timeStamp);
                }
                ext->release();
                break;
            }
        }
    }
    g_free(type);
}

void FileRecoverersExtensionPoint::discardFile(const char *fileUri,
                                               long timeStamp,
                                               const char *fileMimeType)
{
    char *type = g_content_type_from_mime_type(fileMimeType);
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
                ext->discardFile(fileUri, timeStamp);
                ext->release();
                break;
            }
        }
    }
    g_free(type);
}

}
