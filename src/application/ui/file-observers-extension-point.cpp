// Extension point: file observers.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-observers-extension-point.hpp"
#include "file-observer-extension.hpp"
#include "file-observer.hpp"
#include "file.hpp"
#include "application.hpp"
#include "utilities/extension-point-manager.hpp"
#include "utilities/plugin-manager.hpp"
#include <assert.h>
#include <string.h>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <boost/bind.hpp>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <libxml/tree.h>

#define FILE_OBSERVERS "file-observers"
#define MIME_TYPE "mime-type"

namespace Samoyed
{

void FileObserversExtensionPoint::registerExtensionInternally(
    File &file,
    ExtensionInfo &extInfo,
    bool openFile)
{
    char *type = g_content_type_from_mime_type(file.mimeType());
    if (g_content_type_is_a(type, extInfo.type.c_str()))
    {
        FileObserverExtension *ext =
            static_cast<FileObserverExtension *>(
                Application::instance().pluginManager().
                acquireExtension(extInfo.id.c_str()));
        if (!ext)
            return;
        FileObserver *ob = ext->activateObserver(file);
        ext->release();
        if (openFile)
            ob->onFileOpened();
    }
    g_free(type);
}

FileObserversExtensionPoint::FileObserversExtensionPoint():
    ExtensionPoint(FILE_OBSERVERS)
{
    Application::instance().extensionPointManager().
        registerExtensionPoint(*this);

    m_filesOpenedConnection = File::addOpenedCallback(boost::bind(
        &FileObserversExtensionPoint::registerAllExtensionsOnFileOpened,
        this, _1));
}

FileObserversExtensionPoint::~FileObserversExtensionPoint()
{
    m_filesOpenedConnection.disconnect();

    Application::instance().extensionPointManager().
        unregisterExtensionPoint(*this);

    assert(m_extensions.empty());
}

void FileObserversExtensionPoint::registerAllExtensionsOnFileOpened(File &file)
{
    for (ExtensionTable::const_iterator it = m_extensions.begin();
         it != m_extensions.end();
         ++it)
        registerExtensionInternally(file, *it->second, true);
}

bool FileObserversExtensionPoint::registerExtension(const char *extensionId,
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
        cp = g_strdup_printf(
            _("Line %d: Incomplete file observer extension specification.\n"),
            xmlNode->line);
        errors.push_back(cp);
        g_free(cp);
        delete ext;
        return false;
    }
    m_extensions.insert(std::make_pair(ext->id.c_str(), ext));

    // Register the file observer to each opened file.
    for (File *file = Application::instance().files();
         file;
         file = file->next())
        registerExtensionInternally(*file, *ext, false);

    return true;
}

void FileObserversExtensionPoint::unregisterExtension(const char *extensionId)
{
    ExtensionInfo *ext = m_extensions[extensionId];
    m_extensions.erase(extensionId);
    delete ext;
}

}
