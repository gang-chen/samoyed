// Preferences editor.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_PREFERENCES_EDITOR_HPP
#define SMYD_PREFERENCES_EDITOR_HPP

#include <map>
#include <string>
#include <boost/signals2/signal.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class PreferencesEditor
{
public:
    typedef boost::signals2::signal<void (GtkWidget *grid)> Setup;

    static void registerPreferences(const char *category,
                                    const Setup::slot_type &setup);

    static PreferencesEditor &instance();

    void show();

    void hide();

private:
    static std::map<std::string, Setup *> s_preferences;

    static PreferencesEditor *s_instance;

    PreferencesEditor();

    GtkWidget *m_window;
};

}

#endif
