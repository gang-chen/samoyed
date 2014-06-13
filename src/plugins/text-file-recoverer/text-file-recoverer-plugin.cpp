// Plugin: text file recoverer.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-recoverer-plugin.hpp"
#include "text-file-recoverer-extension.hpp"
#include "text-file-observer-extension.hpp"
#include <string.h>

namespace Samoyed
{

namespace TextFileRecoverer
{

Extension *TextFileRecovererPlugin::createExtension(const char *extensionId)
{
    if (strcmp(extensionId, "text-file-recoverer/file-recoverer") == 0)
        return new TextFileRecovererExtension(extensionId, *this);
    if (strcmp(extensionId, "text-file-recoverer/file-observer" == 0)
        return new TextFileObserverExtension(externsionId, *this);
    return NULL;
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
