// Preferences editor.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "preferences-editor.hpp"
#include <map>
#include <string>
#include <utility>
#include <boost/signals2/signal.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

std::map<std::string, PreferencesEditor::Setup *>
    PreferencesEditor::s_preferences;

PreferencesEditor *PreferencesEditor::s_instance = NULL;

void PreferencesEditor::registerPreferences(const char *category,
                                            const Setup::slot_type &setup)
{
    std::map<std::string, Setup *>::iterator it = s_preferences.find(category);
    if (it == s_preferences.end())
        it = s_preferences.insert(std::make_pair(category, new Setup)).first;
    it->second->connect(setup);
}

PreferencesEditor &PreferencesEditor::instance()
{
    if (!s_instance)
        s_instance = new PreferencesEditor;
    return *s_instance;
}

PreferencesEditor::PreferencesEditor()
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
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
