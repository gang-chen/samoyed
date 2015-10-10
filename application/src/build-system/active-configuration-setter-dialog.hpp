// Active configuration setter dialog.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_ACTIVE_CONFIGURATION_SETTER_DIALOG_HPP
#define SMYD_ACTIVE_CONFIGURATION_SETTER_DIALOG_HPP

#include <boost/utility.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class BuildSystem;

class ActiveConfigurationSetterDialog: public boost::noncopyable
{
public:
    ActiveConfigurationSetterDialog(GtkWindow *parent,
                                    BuildSystem &buildSystem);

    ~ActiveConfigurationSetterDialog();

    void run();

private:
    static void validateInput(gpointer object,
                              ActiveConfigurationSetterDialog *dialog);

    BuildSystem &m_buildSystem;

    GtkBuilder *m_builder;
    GtkDialog *m_dialog;
    GtkComboBoxText *m_configChooser;
};

}

#endif
