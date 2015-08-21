// Project creator dialog.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_CREATOR_DIALOG_HPP
#define SMYD_PROJECT_CREATOR_DIALOG_HPP

#include <boost/utility.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class Project;

class ProjectCreatorDialog: public boost::noncopyable
{
public:
    ProjectCreatorDialog(GtkWindow *parent);

    ~ProjectCreatorDialog();

    Project *run();

private:
    static void validateInput(gpointer object, ProjectCreatorDialog *dialog);

    GtkBuilder *m_builder;

    GtkDialog *m_dialog;
    GtkFileChooser *m_locationChooser;
    GtkComboBoxText *m_buildSystemChooser;
};

}

#endif
