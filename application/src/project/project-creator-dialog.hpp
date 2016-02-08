// Project creator dialog.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_CREATOR_DIALOG_HPP
#define SMYD_PROJECT_CREATOR_DIALOG_HPP

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class ProjectCreatorDialog: public boost::noncopyable
{
public:
    ProjectCreatorDialog(GtkWindow *parent);

    ~ProjectCreatorDialog();

    bool run();

    boost::shared_ptr<char> projectUri() const;

    const char *projectBuildSystem() const;

private:
    static void validateInput(ProjectCreatorDialog *dialog);

    GtkBuilder *m_builder;

    GtkDialog *m_dialog;
    GtkFileChooser *m_locationChooser;
    GtkComboBoxText *m_buildSystemChooser;
};

}

#endif
