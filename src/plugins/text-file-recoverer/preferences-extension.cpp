// Extension: preferences.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "preferences-extension.hpp"
#include "application.hpp"
#include "utilities/property-tree.hpp"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

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

void PreferencesExtension::uninstallPreferences()
{
    Application::instance().preferences().child(TEXT_EDITOR).
        removeChild(TEXT_EDIT_SAVE_INTERVAL);
}

void PreferencesExtension::setupPreferencesEditor(const char *category,
                                                  GtkGrid *grid)
{
    GtkWidget *g = gtk_grid_new();
    GtkWidget *label1 = gtk_label_new(_("Auto save files every"));
    GtkWidget *entry = gtk_entry_new();
    GtkWidget *label2 = gtk_label_new(_("seconds"));
    gtk_grid_attach(GTK_GRID(g), label1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(g), entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(g), label2, 2, 0, 1, 1);
    gtk_grid_set_column_spacing(GTK_GRID(g), CONTAINER_SPACING);
    gtk_grid_attach_next_to(grid, g, NULL, GTK_POS_BOTTOM, 1, 1);
    gtk_widget_show_all(g);
}

}

}
