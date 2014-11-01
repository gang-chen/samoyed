// Extension: text file recoverer preferences.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "text-file-recoverer-preferences-extension.hpp"
#include "text-file-recoverer-plugin.hpp"
#include "application.hpp"
#include "utilities/property-tree.hpp"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#define TEXT_EDITOR "text-editor"
#define SAVE_TEXT_EDITS "save-text-edits"
#define TEXT_EDIT_SAVE_INTERVAL "text-edit-save-interval"

namespace
{

const bool DEFAULT_SAVE_TEXT_EDITS = true;

const int DEFAULT_TEXT_EDIT_SAVE_INTERVAL = 5;

void onSaveToggled(GtkToggleButton *toggle, gpointer data)
{
    Samoyed::PropertyTree &prefs =
        Samoyed::Application::instance().preferences().child(TEXT_EDITOR);
    prefs.set(SAVE_TEXT_EDITS,
              static_cast<bool>(gtk_toggle_button_get_active(toggle)),
              false,
              NULL);
    if (!prefs.get<bool>(SAVE_TEXT_EDITS))
        Samoyed::TextFileRecoverer::TextFileRecovererPlugin::instance().
            deactivateTextEditSavers();
}

void onSaveIntervalChanged(GtkSpinButton *spin, gpointer data)
{
    Samoyed::Application::instance().preferences().child(TEXT_EDITOR).
        set(TEXT_EDIT_SAVE_INTERVAL,
            static_cast<int>(gtk_spin_button_get_value_as_int(spin)),
            false,
            NULL);
}

}

namespace Samoyed
{

namespace TextFileRecoverer
{

void TextFileRecovererPreferencesExtension::installPreferences()
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    prefs.addChild(SAVE_TEXT_EDITS, DEFAULT_SAVE_TEXT_EDITS);
    prefs.addChild(TEXT_EDIT_SAVE_INTERVAL, DEFAULT_TEXT_EDIT_SAVE_INTERVAL);
}

void TextFileRecovererPreferencesExtension::uninstallPreferences()
{
    PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    PropertyTree &saveTextEdits = prefs.child(SAVE_TEXT_EDITS);
    prefs.removeChild(saveTextEdits);
    delete &saveTextEdits;
    PropertyTree &saveInterval = prefs.child(TEXT_EDIT_SAVE_INTERVAL);
    prefs.removeChild(saveInterval);
    delete &saveInterval;
}

void TextFileRecovererPreferencesExtension::setupPreferencesEditor(
    const char *category,
    GtkGrid *grid)
{
    const PropertyTree &prefs =
        Application::instance().preferences().child(TEXT_EDITOR);
    GtkWidget *check = gtk_check_button_new();
    GtkWidget *g = gtk_grid_new();
    GtkWidget *label1 = gtk_label_new_with_mnemonic(
        _("_Save changed files every"));
    GtkAdjustment *adjust = gtk_adjustment_new(
        prefs.get<int>(TEXT_EDIT_SAVE_INTERVAL),
        1.0, 100.0, 1.0, 5.0, 0.0);
    GtkWidget *spin = gtk_spin_button_new(adjust, 1.0, 0);
    GtkWidget *label2 = gtk_label_new(
        _("minutes in replay files for recovering them from crashes"));
    gtk_grid_attach(GTK_GRID(g), label1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(g), spin, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(g), label2, 2, 0, 1, 1);
    gtk_grid_set_column_spacing(GTK_GRID(g), CONTAINER_SPACING);
    gtk_container_add(GTK_CONTAINER(check), g);
    gtk_grid_attach_next_to(grid, check, NULL, GTK_POS_BOTTOM, 1, 1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check),
                                 prefs.get<bool>(SAVE_TEXT_EDITS));
    g_signal_connect(check, "toggled",
                     G_CALLBACK(onSaveToggled), NULL);
    g_signal_connect(spin, "value-changed",
                     G_CALLBACK(onSaveIntervalChanged), NULL);
    gtk_widget_show_all(check);
}

}

}
