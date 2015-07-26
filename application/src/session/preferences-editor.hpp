// Preferences editor.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_PREFERENCES_EDITOR_HPP
#define SMYD_PREFERENCES_EDITOR_HPP

#include <string>
#include <vector>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class PreferencesEditor: public boost::noncopyable
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

private:
    struct Category
    {
        std::string id;
        std::string label;
        std::vector<Setup> preferences;
        Category(const char *id, const char *label):
            id(id), label(label)
        {}
    };

    static gboolean setupCategoryPage(gpointer param);

    static std::vector<Category> s_categories;

    GtkWidget *m_window;
};

}

#endif
