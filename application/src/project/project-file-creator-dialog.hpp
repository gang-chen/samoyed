// Project-managed file creator dialog.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_PROJECT_FILE_CREATOR_DIALOG
#define SMYD_PROJECT_FILE_CREATOR_DIALOG

#include "project-file.hpp"
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class ProjectFileCreatorDialog: public boost::noncopyable
{
public:
    ProjectFileCreatorDialog(GtkWindow *parent,
                             Project &project,
                             int type,
                             const char *currentDir);

    ~ProjectFileCreatorDialog();

    boost::shared_ptr<ProjectFile> run();

private:
    void validateInput();

    Project &m_project;

    GtkBuilder *m_builder;
    GtkDialog *m_dialog;
    GtkFileChooser *m_locationChooser;
    GtkEntry *m_nameEntry;

    ProjectFile *m_projectFile;

    ProjectFile::Editor *m_projectFileEditor;
};

}

#endif
