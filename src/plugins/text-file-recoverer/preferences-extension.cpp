// Extension: preferences.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "preferences-extension.hpp"
#include "application.hpp"
#include "utilities/property-tree.hpp"
#include <gtk/gtk.h>

#define TEXT_EDITOR "text-editor"
#define TEXT_EDIT_SAVE_INTERVAL "text-edit-save-interval"

namespace
{

const int DEFAULT_TEXT_EDIT_SAVE_INTERVAL = 300000;

}

namespace Samoyed
{

namespace TextFileRecoverer
{

void PreferencesExtension::installPreferences()
{
    Application::instance().preferences().child(TEXT_EDITOR).
        addChild(TEXT_EDIT_SAVE_INTERVAL, DEFAULT_TEXT_EDIT_SAVE_INTERVAL);
}

void PreferencesExtension::setupPreferencesEditor(const char *category,
                                                  GtkWidget *grid)
{
}

}

}
