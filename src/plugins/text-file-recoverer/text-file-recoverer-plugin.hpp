// Plugin: text file recoverer.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_FILE_RECOVERER_PLUGIN_HPP
#define SMYD_TXTR_TEXT_FILE_RECOVERER_PLUGIN_HPP

#include "utilities/plugin.hpp"

namespace Samoyed
{

namespace TextFileRecoverer
{

class TextFileRecovererPlugin: public Plugin
{
public:
    TextFileRecovererPlugin(PluginManager &manager,
                            const char *id,
                            GModule *module);

    virtual void deactivate();

protected:
    virtual Extension *createExtension(const char *extensionId);

    virtual bool completed() const;
};

}

}

#endif
