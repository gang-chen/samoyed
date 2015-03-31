// Preferences editor.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "preferences-editor.hpp"
#include "ui/preferences-extension-point.hpp"
#include "application.hpp"
#include "utilities/extension-point-manager.hpp"
#include "utilities/miscellaneous.hpp"
#include <string>
#include <utility>
#include <vector>
#include <boost/function.hpp>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#define PREFERENCES "preferences"

namespace
{

const double DEFAULT_SIZE_RATIO = 0.5;

}

namespace Samoyed
{

std::vector<PreferencesEditor::Category> PreferencesEditor::s_categories;

void PreferencesEditor::addCategory(const char *id, const char *label)
{
    s_categories.push_back(Category(id, label));
}

void PreferencesEditor::removeCategory(const char *id)
{
    for (std::vector<Category>::iterator it = s_categories.begin();
         it != s_categories.end();
         ++it)
        if (it->id == id)
        {
            s_categories.erase(it);
            break;
        }
}

void PreferencesEditor::registerPreferences(const char *category,
                                            const Setup &setup)
{
    for (std::vector<Category>::iterator it = s_categories.begin();
         it != s_categories.end();
         ++it)
        if (it->id == category)
        {
            it->preferences.push_back(setup);
            break;
        }
}

gboolean PreferencesEditor::setupCategoryPage(gpointer param)
{
    std::pair<PreferencesEditor *, GtkWidget *> *p =
        static_cast<std::pair<PreferencesEditor *, GtkWidget *> *>(param);
    gtk_container_remove(GTK_CONTAINER(p->second),
                         gtk_grid_get_child_at(GTK_GRID(p->second), 0, 0));
    GtkWidget *c = gtk_widget_get_parent(gtk_widget_get_parent(p->second));
    GtkWidget *n = gtk_bin_get_child(GTK_BIN(p->first->m_window));
    int i = gtk_notebook_page_num(GTK_NOTEBOOK(n), c);
    for (std::vector<Setup>::iterator it =
            s_categories[i].preferences.begin();
         it != s_categories[i].preferences.end();
         ++it)
        (*it)(GTK_GRID(p->second));
    static_cast<PreferencesExtensionPoint &>(Application::instance().
        extensionPointManager().extensionPoint(PREFERENCES)).
        setupPreferencesEditor(s_categories[i].id.c_str(),
                               GTK_GRID(p->second));
    delete p;
    return FALSE;
}

void PreferencesEditor::onCategoryPageMapped(GtkWidget *grid,
                                             PreferencesEditor *editor)
{
    g_idle_add(setupCategoryPage,
               new std::pair<PreferencesEditor *, GtkWidget *>(editor, grid));
}

PreferencesEditor::PreferencesEditor()
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(m_window), notebook);
    gtk_container_set_border_width(GTK_CONTAINER(m_window),
                                   CONTAINER_BORDER_WIDTH);
    for (std::vector<Category>::iterator it = s_categories.begin();
         it != s_categories.end();
         ++it)
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
        GtkWidget *label = gtk_label_new_with_mnemonic(it->label.c_str());
        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sw, label);
        gtk_container_set_border_width(GTK_CONTAINER(grid),
                                       CONTAINER_BORDER_WIDTH);
        gtk_grid_set_row_spacing(GTK_GRID(grid), CONTAINER_SPACING);
        g_signal_connect(grid, "map", G_CALLBACK(onCategoryPageMapped), this);
    }
    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(m_window));
    int width = gdk_screen_get_width(screen) * DEFAULT_SIZE_RATIO;
    int height = gdk_screen_get_height(screen) * DEFAULT_SIZE_RATIO;
    gtk_window_set_default_size(GTK_WINDOW(m_window), width, height);
    gtk_window_set_title(GTK_WINDOW(m_window), _("Preferences"));
    g_signal_connect(m_window, "delete-event",
                     G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    gtk_widget_show_all(notebook);
}

PreferencesEditor::~PreferencesEditor()
{
    gtk_widget_destroy(m_window);
}

void PreferencesEditor::show()
{
    gtk_window_present(GTK_WINDOW(m_window));
}

void PreferencesEditor::hide()
{
    gtk_widget_hide(m_window);
}

}
