// Extension: preferences.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_TXTR_PREFERENCES_EXTENSION_HPP
#define SMYD_TXTR_PREFERENCES_EXTENSION_HPP

#include "ui/preferences-extension.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

namespace TextFileRecoverer
{

class PreferencesExtension: public Samoyed::PreferencesExtension
{
public:
    PreferencesExtension(const char *id, Plugin &plugin):
        Samoyed::PreferencesExtension(id, plugin)
    {}

    virtual void installPreferences();

    virtual void uninstallPreferences();

    virtual void setupPreferencesEditor(const char *category,
                                         GtkGrid *grid);
};

}

}

#endif
