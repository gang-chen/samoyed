// Plugin manager.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "plugin-manager.hpp"
#include "plugin.hpp"
#include "extension-point-manager.hpp"
#include "extension-point.hpp"
#include "miscellaneous.hpp"
#include <string.h>
#include <utility>
#include <glib.h>
#include <glib/gi18n-lib.h>
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

namespace
{

const int CACHE_SIZE = 8;

}

namespace Samoyed
{

PluginManager::PluginManager(ExtensionPointManager &extensionPointMgr,
                             const char *modulesDirName,
                             int cacheSize):
    m_extensionPointManager(extensionPointMgr),
    m_modulesDirName(modulesDirName),
    m_cacheSize(cacheSize),
    m_nCachedPlugins(0),
    m_lruCachedPlugin(NULL),
    m_mruCachedPlugin(NULL)
{
}

PluginManager::~PluginManager()
{
    for (Table::iterator it = m_table.begin(); it != m_table.end();)
    {
        Table::iterator it2 = it;
        ++it;
        Plugin *plugin = it2->second;
        m_table.erase(it2);
        plugin->destroy();
    }
    for (Registry::iterator it = m_registry.begin(); it != m_registry.end();)
    {
        Registry::iterator it2 = it;
        ++it;
        PluginInfo *info = it2->second;
        m_registry.erase(it2);
        delete info;
    }
}

const PluginManager::PluginInfo *
PluginManager::findPluginInfo(const char *pluginId) const
{
    Registry::const_iterator it = m_registry.find(pluginId);
    if (it == m_registry.end())
        return NULL;
    return it->second;
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
    info->unregister = false;
    info->enabled = true;
    char *dirName = g_path_get_dirname(pluginManifestFileName);
    info->directoryName = dirName;
    g_free(dirName);
    info->extensions = NULL;
    info->xmlDoc = doc;

    char *value;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name), ID) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->id = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        NAME) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->name = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        DESCRIPTION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->description = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        MODULE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                info->module = value;
            xmlFree(value);
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
                        xmlNodeListGetString(doc, grandChild->children, 1));
                    if (value)
                        ext->id = info->id + '/' + value;
                    xmlFree(value);
                }
                else if (strcmp(reinterpret_cast<const char *>(grandChild->name),
                                POINT) == 0)
                {
                    value = reinterpret_cast<char *>(
                        xmlNodeListGetString(doc, grandChild->children, 1));
                    if (value)
                        ext->pointId = value;
                    xmlFree(value);
                }
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
            else
            {
                ext->xmlNode = child;
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
        xmlFreeDoc(info->xmlDoc);
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
        xmlFreeDoc(info->xmlDoc);
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
        xmlFreeDoc(info->xmlDoc);
        delete info;
        return false;
    }

    // Register the plugin.
    m_registry.insert(std::make_pair(info->id.c_str(), info));

    // Register the extensions.
    for (PluginInfo::ExtensionInfo *ext = info->extensions;
         ext;
         ext = ext->next)
    {
        m_extensionPointManager.
            registerExtension(ext->id.c_str(),
                              ext->pointId.c_str(),
                              doc,
                              ext->xmlNode);
    }

    return true;
}

void PluginManager::unregisterPluginInternally(PluginInfo &pluginInfo)
{
    for (PluginInfo::ExtensionInfo *ext = pluginInfo.extensions;
         ext;
         ext = ext->next)
    {
        m_extensionPointManager.
            unregisterExtension(ext->id.c_str(),
                                ext->pointId.c_str());
    }
    m_registry.erase(pluginInfo.id.c_str());
    delete &pluginInfo;
}

void PluginManager::unregisterPlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (info->unregister)
        return;

    // Mark the plugin as to be unregistered.
    info->unregister = true;

    // If the plugin is active, try to deactivate it.
    Table::const_iterator it = m_table.find(pluginId);
    if (it != m_table.end())
        it->second->deactivate();
    else
        unregisterPluginInternally(*info);
}

void PluginManager::enablePlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (info->unregister || info->enabled)
        return;

    // Mark the plugin as enabled.
    info->enabled = true;

    // Activate extensions.
    for (PluginInfo::ExtensionInfo *ext = info->extensions;
         ext;
         ext = ext->next)
    {
        ExtensionPoint *point = m_extensionPointManager.
            extensionPoint(ext->pointId.c_str());
        if (point)
            point->onExtensionEnabled(ext->id.c_str());
    }
}

void PluginManager::disablePlugin(const char *pluginId)
{
    PluginInfo *info = m_registry.find(pluginId)->second;
    if (info->unregister || !info->enabled)
        return;

    // Mark the plugin as disabled.
    info->enabled = false;

    // If the plugin is active, try to deactivate it.
    Table::const_iterator it = m_table.find(pluginId);
    if (it != m_table.end())
        it->second->deactivate();
}

Extension *PluginManager::acquireExtension(const char *extensionId,
                                           ExtensionPoint &extensionPoint)
{
    std::string pluginId(extensionId, strrchr(extensionId, '/') - extensionId);

    PluginInfo *info = m_registry.find(pluginId.c_str())->second;
    if (info->unregister || !info->enabled)
        return NULL;

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
    Extension *ext = plugin->acquireExtension(extensionId, extensionPoint);
    if (!ext && newPlugin)
    {
        plugin->addToCache(m_lruCachedPlugin, m_mruCachedPlugin);
        if (++m_nCachedPlugins > m_cacheSize)
        {
            Plugin *p = m_lruCachedPlugin;
            m_table.erase(p->id());
            p->removeFromCache(m_lruCachedPlugin, m_mruCachedPlugin);
            p->destroy();
            --m_nCachedPlugins;
        }
    }
    return ext;
}

void PluginManager::deactivatePlugin(Plugin &plugin)
{
    std::string pluginId(plugin.id());

    plugin.addToCache(m_lruCachedPlugin, m_mruCachedPlugin);
    if (++m_nCachedPlugins > m_cacheSize)
    {
        Plugin *p = m_lruCachedPlugin;
        m_table.erase(p->id());
        p->removeFromCache(m_lruCachedPlugin, m_mruCachedPlugin);
        p->destroy();
        --m_nCachedPlugins;
    }

    // If requested, unregister the plugin after it is deactivated.
    PluginInfo *info = m_registry.find(pluginId.c_str())->second;
    if (info->unregister)
        unregisterPluginInternally(*info);
}

void PluginManager::scanPlugins(const char *pluginsDirName)
{
    GError *error = NULL;
    GDir *dir = g_dir_open(pluginsDirName, 0, &error);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().windows() ?
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()) :
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
