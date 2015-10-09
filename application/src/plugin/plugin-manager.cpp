// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "plugin-manager.hpp"
#include "plugin.hpp"
#include "extension-point-manager.hpp"
#include "utilities/miscellaneous.hpp"
#include <string.h>
#include <utility>
#include <libintl.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <libxml/xmlerror.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#define PLUGIN "plugin"
#define ID "id"
#define NAME "name"
#define DESCRIPTION "description"
#define MODULE "module"
#define EXTENSION "extension"
#define POINT "point"
#define DETAIL "detail"

namespace Samoyed
{

PluginManager::PluginInfo::~PluginInfo()
{
    for (ExtensionInfo *next; extensions; extensions = next)
    {
        next = extensions->next;
        delete extensions;
    }
    xmlFreeDoc(xmlDoc);
}

gboolean PluginManager::destroyPluginDeferred(gpointer param)
{
    std::pair<PluginManager *, Plugin *> *p =
        static_cast<std::pair<PluginManager *, Plugin *> *>(param);
    p->first->m_table.erase(p->second->id());
    p->second->destroy();
    if (p->first->m_shuttingDown &&
        static_cast<int>(p->first->m_table.size()) ==
            p->first->m_nCachedPlugins)
        delete p->first;
    delete p;
    return FALSE;
}

PluginManager::PluginManager(ExtensionPointManager &extensionPointMgr,
                             const char *modulesDirName,
                             int cacheSize):
    m_extensionPointManager(extensionPointMgr),
    m_modulesDirName(modulesDirName),
    m_cacheSize(cacheSize),
    m_nCachedPlugins(0),
    m_lruCachedPlugin(NULL),
    m_mruCachedPlugin(NULL),
    m_shuttingDown(false)
{
}

PluginManager::~PluginManager()
{
    Table::iterator it;
    while ((it = m_table.begin()) != m_table.end())
    {
        Plugin *plugin = it->second;
        m_table.erase(it);
        plugin->destroy();
    }
}

void PluginManager::shutDown()
{
    if (m_shuttingDown)
        return;
    m_shuttingDown = true;

    // Unregister all plugins.
    Registry::iterator it;
    while ((it = m_registry.begin()) != m_registry.end())
        unregisterPlugin(it->second->id.c_str());

    if (static_cast<int>(m_table.size()) == m_nCachedPlugins)
        delete this;
}

const PluginManager::PluginInfo *
PluginManager::findPluginInfo(const char *pluginId) const
{
    Registry::const_iterator it = m_registry.find(pluginId);
    if (it == m_registry.end())
        return NULL;
    return it->second;
}

void PluginManager::registerPluginExtensions(PluginInfo &pluginInfo)
{
    for (PluginInfo::ExtensionInfo *ext = pluginInfo.extensions;
         ext;
         ext = ext->next)
    {
        m_extensionPointManager.
            registerExtension(ext->id.c_str(),
                              ext->pointId.c_str(),
                              ext->xmlNode);
    }
}

void PluginManager::unregisterPluginExtensions(PluginInfo &pluginInfo)
{
    for (PluginInfo::ExtensionInfo *ext = pluginInfo.extensions;
         ext;
         ext = ext->next)
    {
        m_extensionPointManager.
            unregisterExtension(ext->id.c_str(),
                                ext->pointId.c_str());
    }
}

bool PluginManager::registerPlugin(const char *pluginManifestFileName)
{
    // Parse the plugin manifest file.
    xmlDocPtr doc = xmlParseFile(pluginManifestFileName);
    if (!doc)
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to register a plugin from plugin manifest file "
              "\"%s\"."),
            pluginManifestFileName);
        xmlErrorPtr error = xmlGetLastError();
        if (error)
            Samoyed::gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to parse plugin manifest file \"%s\". %s."),
                pluginManifestFileName, error->message);
        else
            Samoyed::gtkMessageDialogAddDetails(
                dialog,
                _("Samoyed failed to parse plugin manifest file \"%s\"."),
                pluginManifestFileName);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }

    xmlNodePtr node = xmlDocGetRootElement(doc);
    if (!node)
    {
        xmlFreeDoc(doc);
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to register a plugin from plugin manifest file "
              "\"%s\"."),
            pluginManifestFileName);
        Samoyed::gtkMessageDialogAddDetails(
            dialog,
            _("Plugin manifest file \"%s\" is empty."),
            pluginManifestFileName);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }

    if (strcmp(reinterpret_cast<const char *>(node->name), PLUGIN) != 0)
    {
        xmlFreeDoc(doc);
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to register a plugin from plugin manifest file "
              "\"%s\"."),
            pluginManifestFileName);
        Samoyed::gtkMessageDialogAddDetails(
            dialog,
            _("Line %d: Root element is \"%s\"; should be \"%s\".\n"),
            node->line,
            reinterpret_cast<const char *>(node->name),
            PLUGIN);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }

    PluginInfo *info = new PluginInfo;
    char *dirName = g_path_get_dirname(pluginManifestFileName);
    info->directoryName = dirName;
    g_free(dirName);
    info->extensions = NULL;
    info->xmlDoc = doc;
    info->enabled = true;
    info->cache = true;

    char *value;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name), ID) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                info->id = value;
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
                info->name = gettext(value);
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        DESCRIPTION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                info->description = gettext(value);
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        MODULE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                info->module = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        EXTENSION) == 0)
        {
            PluginInfo::ExtensionInfo *ext = new PluginInfo::ExtensionInfo;
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
                        ext->id = info->id + '/' + value;
                        xmlFree(value);
                    }
                }
                else if (strcmp(reinterpret_cast<const char *>
                                    (grandChild->name),
                                POINT) == 0)
                {
                    value = reinterpret_cast<char *>(
                        xmlNodeGetContent(grandChild->children));
                    if (value)
                    {
                        ext->pointId = value;
                        xmlFree(value);
                    }
                }
                else if (strcmp(reinterpret_cast<const char *>
                                    (grandChild->name),
                                DETAIL) == 0)
                    ext->xmlNode = grandChild;
            }
            if (ext->id.empty())
            {
                GtkWidget *dialog;
                dialog = gtk_message_dialog_new(
                    NULL,
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    _("Samoyed found an error in plugin manifest file \"%s\"."),
                    pluginManifestFileName);
                Samoyed::gtkMessageDialogAddDetails(
                    dialog,
                    _("Line %d: Identifier missing for extension; extension "
                      "skipped.\n"),
                    child->line);
                gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                                GTK_RESPONSE_CLOSE);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                delete ext;
            }
            else if (ext->pointId.empty())
            {
                GtkWidget *dialog;
                dialog = gtk_message_dialog_new(
                    NULL,
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    _("Samoyed found an error in plugin manifest file \"%s\"."),
                    pluginManifestFileName);
                Samoyed::gtkMessageDialogAddDetails(
                    dialog,
                    _("Line %d: Extension point missing for extension \"%s\"; "
                      "extension skipped.\n"),
                    child->line,
                    ext->id.c_str());
                gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                                GTK_RESPONSE_CLOSE);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                delete ext;
            }
            else if (!ext->xmlNode)
            {
                GtkWidget *dialog;
                dialog = gtk_message_dialog_new(
                    NULL,
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    _("Samoyed found an error in plugin manifest file \"%s\"."),
                    pluginManifestFileName);
                Samoyed::gtkMessageDialogAddDetails(
                    dialog,
                    _("Line %d: Extension detail missing for extension \"%s\"; "
                      "extension skipped.\n"),
                    child->line,
                    ext->id.c_str());
                gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                                GTK_RESPONSE_CLOSE);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                delete ext;
            }
            else
            {
                ext->next = info->extensions;
                info->extensions = ext;
            }
        }
    }

    if (info->id.empty())
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to register a plugin from plugin manifest file "
              "\"%s\"."),
            pluginManifestFileName);
        Samoyed::gtkMessageDialogAddDetails(
            dialog,
            _("Identifier missing for plugin.\n"));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        delete info;
        return false;
    }
    if (info->module.empty())
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to register a plugin from plugin manifest file "
              "\"%s\"."),
            pluginManifestFileName);
        Samoyed::gtkMessageDialogAddDetails(
            dialog,
            _("Module missing for plugin \"%s\".\n"),
            info->id.c_str());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        delete info;
        return false;
    }
    if (!info->extensions)
    {
        GtkWidget *dialog;
        dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to register a plugin from plugin manifest file "
              "\"%s\"."),
            pluginManifestFileName);
        Samoyed::gtkMessageDialogAddDetails(
            dialog,
            _("No extension provided by plugin \"%s\".\n"),
            info->id.c_str());
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        delete info;
        return false;
    }

    // Register the plugin.
    m_registry.insert(std::make_pair(info->id.c_str(), info));

    // Register the extensions.
    registerPluginExtensions(*info);

    return true;
}

void PluginManager::unregisterPlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;

    // Unregister the extensions.
    unregisterPluginExtensions(*info);

    // Unregister the plugin.
    m_registry.erase(pluginId);
    delete info;

    // Do not cache the plugin.
    info->cache = false;

    // If the plugin is active, deactivate it.
    Table::const_iterator it = m_table.find(pluginId);
    if (it != m_table.end())
        it->second->deactivate();
}

void PluginManager::enablePlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (!info->enabled)
    {
        info->enabled = true;
        registerPluginExtensions(*info);
    }
}

void PluginManager::disablePlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (info->enabled)
    {
        info->enabled = false;
        unregisterPluginExtensions(*info);

        info->cache = false;

        Table::const_iterator it = m_table.find(pluginId);
        if (it != m_table.end())
            it->second->deactivate();
    }
}

Extension *PluginManager::acquireExtension(const char *extensionId)
{
    std::string pluginId(extensionId, strrchr(extensionId, '/') - extensionId);
    PluginInfo *info = m_registry.find(pluginId.c_str())->second;

    // Retrieve the plugin if it is active, fetch the plugin from the cache if
    // it is cached or create it.
    Plugin *plugin;
    bool newPlugin;
    Table::const_iterator it = m_table.find(pluginId.c_str());
    if (it != m_table.end())
    {
        plugin = it->second;
        if (plugin->cached())
        {
            plugin->removeFromCache(m_lruCachedPlugin, m_mruCachedPlugin);
            --m_nCachedPlugins;
            newPlugin = true;
        }
        else
            newPlugin = false;
    }
    else
    {
        char *module = g_module_build_path(m_modulesDirName.c_str(),
                                           info->module.c_str());
        std::string error;
        plugin = Plugin::activate(*this, pluginId.c_str(), module, error);
        g_free(module);
        if (!plugin)
        {
            GtkWidget *dialog;
            dialog = gtk_message_dialog_new(
                NULL,
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                _("Samoyed failed to activate plugin \"%s\"."),
                pluginId.c_str());
            if (!error.empty())
                Samoyed::gtkMessageDialogAddDetails(
                    dialog,
                    _("%s"),
                    error.c_str());
            gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                            GTK_RESPONSE_CLOSE);
            gtk_dialog_run(GTK_DIALOG(dialog));
            gtk_widget_destroy(dialog);
            return NULL;
        }
        m_table.insert(std::make_pair(plugin->id(), plugin));
        newPlugin = true;
    }

    // Acquire the extension from the plugin.
    Extension *ext = plugin->acquireExtension(extensionId);
    if (!ext && newPlugin)
    {
        plugin->addToCache(m_lruCachedPlugin, m_mruCachedPlugin);
        if (++m_nCachedPlugins > m_cacheSize)
        {
            Plugin *p = m_lruCachedPlugin;
            m_table.erase(p->id());
            p->removeFromCache(m_lruCachedPlugin, m_mruCachedPlugin);
            --m_nCachedPlugins;
            // Defer destroying the plugin.
            g_idle_add_full(G_PRIORITY_LOW,
                            destroyPluginDeferred,
                            new std::pair<PluginManager *, Plugin *>(this, p),
                            NULL);
        }
    }
    return ext;
}

void PluginManager::destroyPlugin(Plugin &plugin)
{
    const PluginInfo *info = findPluginInfo(plugin.id());

    // Cache the plugin only if it can be reused later.
    if (info && info->cache)
    {
        plugin.addToCache(m_lruCachedPlugin, m_mruCachedPlugin);
        if (++m_nCachedPlugins > m_cacheSize)
        {
            Plugin *p = m_lruCachedPlugin;
            m_table.erase(p->id());
            p->removeFromCache(m_lruCachedPlugin, m_mruCachedPlugin);
            --m_nCachedPlugins;
            // Defer destroying the plugin.
            g_idle_add_full(G_PRIORITY_LOW,
                            destroyPluginDeferred,
                            new std::pair<PluginManager *, Plugin *>(this, p),
                            NULL);
        }
    }
    else
        // Defer destroying the plugin.
        g_idle_add_full(G_PRIORITY_LOW,
                        destroyPluginDeferred,
                        new std::pair<PluginManager *, Plugin *>(this, &plugin),
                        NULL);
}

void PluginManager::scanPlugins(const char *pluginsDirName)
{
    GError *error = NULL;
    GDir *dir = g_dir_open(pluginsDirName, 0, &error);
    if (!dir)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to scan directory \"%s\" for plugins."),
            pluginsDirName);
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to open directory \"%s\" to register plugins. "
              "%s."),
            pluginsDirName, error->message);
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
                                        GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        return;
    }

    const char *pluginDirName;
    while ((pluginDirName = g_dir_read_name(dir)))
    {
        std::string manifestFileName(pluginsDirName);
        manifestFileName += G_DIR_SEPARATOR;
        manifestFileName += pluginDirName;
        manifestFileName += G_DIR_SEPARATOR_S "plugin.xml";
        if (g_file_test(manifestFileName.c_str(), G_FILE_TEST_IS_REGULAR))
            registerPlugin(manifestFileName.c_str());
    }
    g_dir_close(dir);
}

}
