// Preferences editor.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "preferences-editor.hpp"
#include <vector>
#include <boost/function.hpp>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

namespace
{

const char *categoriesLabels[] =
{
    N_("_Text Editor"),
    NULL
};

}

namespace Samoyed
{

std::vector<PreferencesEditor::Setup>
    PreferencesEditor::s_categories[N_CATEGORIES];

void PreferencesEditor::registerPreferences(Category category,
                                            const Setup &setup)
{
    s_categories[category].push_back(setup);
}

gboolean PreferencesEditor::setupCategoryPage()
{

}

void PreferencesEditor::onCategoryPageMapped(GtkWidget *grid,
                                             PreferencesEditor *editor)
{
    g_idle_add(setupCategoryPage);
}

PreferencesEditor::PreferencesEditor()
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(m_window), notebook);
    for (int i = 0; i < N_CATEGORIES; ++i)
    {
        GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
        GtkWidget *grid = gtk_grid_new();
        gtk_widget_hexpand(grid, TRUE);
        gtk_widget_vexpand(grid, TRUE);
        gtk_container_add(GTK_CONTAINER(sw), grid);
        GtkWidget *label = gtk_label_new(gettext(categoriesLabels[i]));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, label);
        g_signal_connect(grid, "map", G_CALLBACK(onCategoryPageMapped), this);
    }
}

PreferencesEditor::~PreferencesEditor()
{
    gtk_widget_destroy(m_window);
}

void PreferencesEditor::show()
{
    gtk_widget_show(m_window);
}

void PreferencesEditor::hide()
{
    gtk_widget_hide(m_window);
}

}
