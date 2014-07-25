// Preferences editor.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "preferences-editor.hpp"
#include "window.hpp"
#include <utility>
#include <vector>
#include <boost/function.hpp>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

namespace
{

const double DEFAULT_SIZE_RATIO = 0.5;

const char *categoriesLabels[] =
{
    N_("_Text Editor"),
    NULL
};

gboolean onDeleteEvent(GtkWidget *widget,
                       GdkEvent *event,
                       Samoyed::PreferencesEditor *editor)
{
    editor->close();
    return TRUE;
}

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

gboolean PreferencesEditor::setupCategoryPage(gpointer param)
{
    return FALSE;
}

void PreferencesEditor::onCategoryPageMapped(GtkWidget *grid,
                                             PreferencesEditor *editor)
{
    g_idle_add(setupCategoryPage,
               new std::pair<PreferencesEditor *, GtkWidget *>(editor, grid));
}

PreferencesEditor::PreferencesEditor(Window &window):
    m_owner(window)
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(m_window), notebook);
    for (int i = 0; i < N_CATEGORIES; ++i)
    {
        GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
        GtkWidget *grid = gtk_grid_new();
        GtkWidget *msg = gtk_label_new(_("Collecting preferences..."));
        gtk_widget_set_halign(msg, GTK_ALIGN_START);
        gtk_widget_set_valign(msg, GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), msg, 0, 0, 1, 1);
        gtk_widget_set_hexpand(grid, TRUE);
        gtk_widget_set_vexpand(grid, TRUE);
        gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(sw), grid);
        GtkWidget *label =
            gtk_label_new_with_mnemonic(gettext(categoriesLabels[i]));
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, label);
        g_signal_connect(grid, "map", G_CALLBACK(onCategoryPageMapped), this);
    }
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(m_window));
    int width = gdk_screen_get_width(screen) * DEFAULT_SIZE_RATIO;
    int height = gdk_screen_get_height(screen) * DEFAULT_SIZE_RATIO;
    gtk_window_set_default_size(GTK_WINDOW(m_window), width, height);
    gtk_window_set_title(GTK_WINDOW(m_window), _("Preferences"));
    g_signal_connect(m_window, "delete-event",
                     G_CALLBACK(onDeleteEvent), this);
    gtk_widget_show_all(notebook);
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

void PreferencesEditor::close()
{
    gtk_widget_destroy(m_window);
    m_owner.onPreferencesEditorClosed();
}

}
