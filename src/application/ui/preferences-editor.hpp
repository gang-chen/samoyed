// Preferences editor.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_PREFERENCES_EDITOR_HPP
#define SMYD_PREFERENCES_EDITOR_HPP

#include <string>
#include <vector>
#include <boost/function.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class PreferencesEditor
{
public:
    typedef boost::function<void (GtkGrid *grid)> Setup;

    static void addCategory(const char *id, const char *label);
    static void removeCategory(const char *id);

    static void registerPreferences(const char *category,
                                    const Setup &setup);

    PreferencesEditor();
    ~PreferencesEditor();

    void show();
    void hide();

    void close();

private:
    struct Category
    {
        std::string m_id;
        std::string m_label;
        std::vector<Setup> m_preferences;
        Category(const char *id, const char *label):
            m_id(id), m_label(label)
        {}
    };

    static gboolean setupCategoryPage(gpointer param);
    static void onCategoryPageMapped(GtkWidget *grid,
                                     PreferencesEditor *editor);

    static std::vector<Category> s_categories;

    GtkWidget *m_window;
};

}

#endif
