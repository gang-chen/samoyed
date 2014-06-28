// Plugin: text file recoverer.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-recoverer-plugin.hpp"
#include "text-file-observer-extension.hpp"
#include "text-file-recoverer-extension.hpp"
#include "text-edit-saver.hpp"
#include "text-file-recoverer.hpp"
#include "application.hpp"
#include "utilities/plugin-manager.hpp"
#include "utilities/property-tree.hpp"
#include <string.h>
#include <glib.h>
#include <gmodule.h>

#define PLUGIN "plugin"
#define ID "id"
#define PREFERENCES "preferences"

namespace
{

const char *TEXT_REPLAY_FILE_PREFIX = ".smyd.txtr.";

}

namespace Samoyed
{

namespace TextFileRecoverer
{

TextFileRecovererPlugin *TextFileRecovererPlugin::s_instance = NULL;

char *TextFileRecovererPlugin::getTextReplayFileName(const char *uri,
                                                     long timeStamp)
{
    char *path = g_filename_from_uri(uri, NULL, NULL);
    char *base = g_path_get_basename(path);
    char *dir = g_path_get_dirname(path);
    char *t = g_strdup_printf("%ld", timeStamp);
    char *newBase = g_strconcat(".",
                                base,
                                TEXT_REPLAY_FILE_PREFIX,
                                t,
                                NULL);
    char *replayFileName = g_build_filename(dir, newBase, NULL);
    g_free(path);
    g_free(base);
    g_free(dir);
    g_free(t);
    g_free(newBase);
    return replayFileName;
}

TextFileRecovererPlugin::TextFileRecovererPlugin(PluginManager& manager,
                                                 const char *id,
                                                 GModule *module):
    Plugin(manager, id, module),
    m_preferences(PREFERENCES)
{
    s_instance = this;
    TextEditSaver::installPreferences();
}

Extension *TextFileRecovererPlugin::createExtension(const char *extensionId)
{
    if (strcmp(extensionId, "text-file-recoverer/file-recoverer") == 0)
        return new TextFileRecovererExtension(extensionId, *this);
    if (strcmp(extensionId, "text-file-recoverer/file-observer") == 0)
        return new TextFileObserverExtension(extensionId, *this);
    return NULL;
}

void TextFileRecovererPlugin::onTextEditSaverCreated(TextEditSaver &saver)
{
    m_savers.insert(&saver);
}

void TextFileRecovererPlugin::onTextEditSaverDestroyed(TextEditSaver &saver)
{
    m_savers.erase(&saver);
    if (completed())
    {
        m_manager.setPluginXmlElement(id(), save());
        onCompleted();
    }
}

void TextFileRecovererPlugin::onTextFileRecoveringBegun(TextFileRecoverer &rec)
{
    m_recoverers.insert(&rec);
}

void TextFileRecovererPlugin::onTextFileRecoveringEnded(TextFileRecoverer &rec)
{
    m_recoverers.erase(&rec);
    if (completed())
    {
        m_manager.setPluginXmlElement(id(), save());
        onCompleted();
    }
}

bool TextFileRecovererPlugin::completed() const
{
    return m_savers.empty() && m_recoverers.empty();
}

void TextFileRecovererPlugin::deactivate()
{
    while (!m_savers.empty())
        (*m_savers.begin())->deactivate();
    while (!m_recoverers.empty())
        (*m_recoverers.begin())->deactivate();
}

xmlNodePtr TextFileRecovererPlugin::save() const
{
    xmlNodePtr node =
        xmlNewNode(NULL,
                   reinterpret_cast<const xmlChar *>(PLUGIN));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(ID),
                    reinterpret_cast<const xmlChar *>(id()));
    xmlAddChild(node, m_preferences.writeXmlElement());
    return node;
}

}

}

extern "C"
{

Samoyed::Plugin *createPlugin(Samoyed::PluginManager &manager,
                              const char *id,
                              GModule *module,
                              std::string &error)
{
    return
        new Samoyed::TextFileRecoverer::TextFileRecovererPlugin(manager,
                                                                id,
                                                                module);
}

}
