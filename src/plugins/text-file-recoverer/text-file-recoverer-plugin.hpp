// Plugin: text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_FILE_RECOVERER_PLUGIN_HPP
#define SMYD_TXTR_TEXT_FILE_RECOVERER_PLUGIN_HPP

#include "utilities/plugin.hpp"
#include <set>
#include <string>

namespace Samoyed
{

namespace TextFileRecoverer
{

class TextEditSaver;
class TextFileRecoverer;

class TextFileRecovererPlugin: public Plugin
{
public:
    static char *getTextReplayFileName(const char *uri, long timeStamp);

    static TextFileRecovererPlugin &instance() { return *s_instance; }

    TextFileRecovererPlugin(PluginManager &manager,
                            const char *id,
                            GModule *module);

    virtual void deactivate();

    void deactivateTextEditSavers();

    void onTextEditSaverCreated(TextEditSaver &saver);
    void onTextEditSaverDestroyed(TextEditSaver &saver);

    void onTextFileRecoveringBegun(TextFileRecoverer &rec);
    void onTextFileRecoveringEnded(TextFileRecoverer &rec);

protected:
    virtual Extension *createExtension(const char *extensionId);

    virtual bool completed() const;
   
private:
    static TextFileRecovererPlugin *s_instance;

    std::set<TextEditSaver *> m_savers;

    std::set<TextFileRecoverer *> m_recoverers;
};

}

}

#endif
