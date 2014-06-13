// Plugin: text file recoverer.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-recoverer-plugin.hpp"
#include "text-file-recoverer-extension.hpp"
#include "text-file-observer-extension.hpp"
#include "text-edit-saver.hpp"
#include <string.h>
#include <list>

namespace Samoyed
{

namespace TextFileRecoverer
{

TextFileRecovererPlugin::TextFileRecovererPlugin(PluginManager& manager,
                                                 const char *id,
                                                 GModule *module):
    Plugin(manager, id, module)
{
}

Extension *TextFileRecovererPlugin::createExtension(const char *extensionId)
{
    if (strcmp(extensionId, "text-file-recoverer/file-recoverer") == 0)
        return new TextFileRecovererExtension(extensionId, *this);
    if (strcmp(extensionId, "text-file-recoverer/file-observer" == 0)
        return new TextFileObserverExtension(extensionId, *this);
    return NULL;
}

void TextFileRecovererPlugin::onTextEditSaverCreated(TextEditSaver &saver)
{
    m_savers.push_back(&saver);
}

void TextFileRecovererPlugin::onTextEditSaverDestroyed(TextEditSaver &saver)
{
    m_savers.erase(std::remove(m_savers.begin(), m_savers.end(), &saver),
                   m_savers.end());
    if (m_savers.empty())
        onCompleted();
}

bool TextFileRecovererPlugin::completed() const
{
    return m_savers.empty();
}

void TextFileRecovererPlugin::deactivate()
{
    while (!m_savers.empty())
        m_savers.front()->deactivate();
}

}

}

extern "C"
{

Samoyed::Plugin *createPluggin(Samoyed::PluginManager &manager,
                               const char *id,
                               GModlue *model)
{
    return
        new Samoyed::TextFileRecoverer::TextFileRecovererPlugin(manager,
                                                                id,
                                                                model);
}

}
