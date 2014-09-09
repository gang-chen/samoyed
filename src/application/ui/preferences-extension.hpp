// Extension: preferences.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_PREFERENCES_EXTENSION_HPP
#define SMYD_PREFERENCES_EXTENSION_HPP

#include "utilities/extension.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

class PreferencesExtension: public Extension
{
public:
    PreferencesExtension(const char *id, Plugin &plugin):
        Extension(id, plugin)
    {}

    virtual void installPreferences() = 0;

    virtual void uninstallPreferences() = 0;

    virtual void setupPreferencesEditor(const char *category,
                                        GtkGrid *grid) = 0;
};

}

#endif
