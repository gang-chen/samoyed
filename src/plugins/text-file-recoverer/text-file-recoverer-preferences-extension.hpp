// Extension: text file recoverer preferences.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_TXRC_TEXT_FILE_RECOVERER_PREFERENCES_EXTENSION_HPP
#define SMYD_TXRC_TEXT_FILE_RECOVERER_PREFERENCES_EXTENSION_HPP

#include "session/preferences-extension.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

namespace TextFileRecoverer
{

class TextFileRecovererPreferencesExtension: public PreferencesExtension
{
public:
    TextFileRecovererPreferencesExtension(const char *id, Plugin &plugin):
        PreferencesExtension(id, plugin)
    {}

    virtual void installPreferences();

    virtual void uninstallPreferences();

    virtual void setupPreferencesEditor(const char *category,
                                        GtkGrid *grid);
};

}

}

#endif
