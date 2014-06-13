// Plugin: text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_FILE_RECOVERER_PLUGIN_HPP
#define SMYD_TXTR_TEXT_FILE_RECOVERER_PLUGIN_HPP

#include "utilities/plugin.hpp"
#include <list>

namespace Samoyed
{

namespace TextFileRecoverer
{

class TextEditSaver;

class TextFileRecovererPlugin: public Plugin
{
public:
    TextFileRecovererPlugin(PluginManager &manager,
                            const char *id,
                            GModule *module);

    virtual void deactivate();

    void onTextEditSaverCreated(TextEditSaver &saver);
    void onTextEditSaverDestroyed(TextEditSaver &saver);

protected:
    virtual Extension *createExtension(const char *extensionId);

    virtual bool completed() const;
    
    std::list<TextEditSaver *> m_savers;
};

}

}

#endif
