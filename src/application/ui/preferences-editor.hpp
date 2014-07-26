// Preferences editor.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_PREFERENCES_EDITOR_HPP
#define SMYD_PREFERENCES_EDITOR_HPP

#include <vector>
#include <boost/function.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class PreferencesEditor
{
public:
    enum Category
    {
        CATEGORY_TEXT_EDITOR,
        N_CATEGORIES
    };

    typedef boost::function<void (GtkWidget *grid)> Setup;

    static void registerPreferences(Category category,
                                    const Setup &setup);

    PreferencesEditor();
    ~PreferencesEditor();

    void show();
    void hide();

    void close();

private:
    static gboolean setupCategoryPage(gpointer param);
    static void onCategoryPageMapped(GtkWidget *grid,
                                     PreferencesEditor *editor);

    static std::vector<Setup> s_categories[N_CATEGORIES];

    GtkWidget *m_window;
};

}

#endif
