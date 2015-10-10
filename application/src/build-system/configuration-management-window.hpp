// Configuration management window.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_CONFIGURATION_MANAGEMENT_WINDOW_HPP
#define SMYD_CONFIGURATION_MANAGEMENT_WINDOW_HPP

#include <boost/utility.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class ConfigurationManagementWindow: public boost::noncopyable
{
public:
    ConfigurationManagementWindow(GtkWindow *parent);

    ~ConfigurationManagementWindow();

    void show();

    void hide();

private:
    void deleteSelectedConfiguration(bool confirm);

    static void onEditConfiguration(GtkButton *button,
                                    ConfigurationManagementWindow *window);

    static void onDeleteConfiguration(GtkButton *button,
                                      ConfigurationManagementWindow *window);

    static gboolean onKeyPress(GtkWidget *widget,
                               GdkEventKey *event,
                               ConfigurationManagementWindow *window);

    static void onProjectSelectionChanged(
        GtkTreeSelection *selection,
        ConfigurationManagementWindow *window);

    static void onConfigurationSelectionChanged(
        GtkTreeSelection *selection,
        ConfigurationManagementWindow *window);

    GtkBuilder *m_builder;
    GtkWindow *m_window;
    GtkTreeView *m_projectList;
    GtkTreeView *m_configurationList;
    GtkDialog *m_editorDialog;
};

}

#endif
