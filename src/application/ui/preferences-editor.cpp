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
/*
const char *categoriesLabels[] =
{
    N_("Text Editor"),
    NULL
};
*/
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

PreferencesEditor::PreferencesEditor()
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
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
