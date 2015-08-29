// Project-managed file creator dialog.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-file-creator-dialog.hpp"
#include "project.hpp"
#include "project-file.hpp"
#include "application.hpp"
#include <stdlib.h>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

namespace Samoyed
{

ProjectFileCreatorDialog::ProjectFileCreatorDialog(GtkWindow *parent,
                                                   Project &project,
                                                   int type,
                                                   const char *currentDir):
    m_project(project)
{
    m_projectFile = project.createProjectFile(type);
    m_projectFileEditor = m_projectFile->createEditor();
    m_projectFileEditor->setChangedCallback(boost::bind(validateInput, this));

    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "project-file-creator-dialog.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());
    m_dialog =
        GTK_DIALOG(gtk_builder_get_object(m_builder,
                                          "project-file-creator-dialog"));
    m_locationChooser =
        GTK_FILE_CHOOSER(gtk_builder_get_object(m_builder, "location-chooser"));
    m_nameEntry = GTK_ENTRY(gtk_builder_get_object(m_builder, "name-entry"));
    GtkGrid *grid =
        GTK_GRID(gtk_builder_get_object(m_builder,
                                        "project-file-creator-grid"));
    m_projectFileEditor->addGtkWidgets(grid);

    if (parent)
    {
        gtk_window_set_transient_for(GTK_WINDOW(m_dialog), parent);
        gtk_window_set_modal(GTK_WINDOW(m_dialog), true);
    }

    switch (type)
    {
    case ProjectFile::TYPE_DIRECTORY:
        gtk_window_set_title(GTK_WINDOW(m_dialog), _("New Directory"));
        break;
    case ProjectFile::TYPE_GENERIC_FILE:
        gtk_window_set_title(GTK_WINDOW(m_dialog), _("New File"));
        break;
    case ProjectFile::TYPE_SOURCE_FILE:
        gtk_window_set_title(GTK_WINDOW(m_dialog), _("New Source File"));
        break;
    case ProjectFile::TYPE_HEADER_FILE:
        gtk_window_set_title(GTK_WINDOW(m_dialog), _("New Header File"));
        break;
    case ProjectFile::TYPE_GENERIC_TARGET:
        gtk_window_set_title(GTK_WINDOW(m_dialog), _("New Target"));
        break;
    case ProjectFile::TYPE_PROGRAM:
        gtk_window_set_title(GTK_WINDOW(m_dialog), _("New Program"));
        break;
    case ProjectFile::TYPE_SHARED_LIBRARY:
        gtk_window_set_title(GTK_WINDOW(m_dialog), _("New Shared Library"));
        break;
    case ProjectFile::TYPE_STATIC_LIBRARY:
        gtk_window_set_title(GTK_WINDOW(m_dialog), _("New Static Library"));
    }

    gtk_label_set_label(
        GTK_LABEL(gtk_builder_get_object(m_builder, "project-uri-label")),
        project.uri());
    gtk_file_chooser_set_current_folder_uri(
        GTK_FILE_CHOOSER(gtk_builder_get_object(m_builder, "location-chooser")),
        currentDir);

    validateInput();
}

ProjectFileCreatorDialog::~ProjectFileCreatorDialog()
{
    gtk_widget_destroy(GTK_WIDGET(m_dialog));
    g_object_unref(m_builder);
    delete m_projectFile;
    delete m_projectFileEditor;
}

boost::shared_ptr<ProjectFile> ProjectFileCreatorDialog::run()
{
    boost::shared_ptr<ProjectFile> projectFile;
    if (gtk_dialog_run(m_dialog) != GTK_RESPONSE_ACCEPT)
        return projectFile;
    char *dir = gtk_file_chooser_get_uri(m_locationChooser);
    const char *name = gtk_entry_get_text(m_nameEntry);
    std::string uri(dir);
    uri += G_DIR_SEPARATOR;
    uri += name;
    g_free(dir);
    m_projectFileEditor->getInput(*m_projectFile);
    if (!m_project.addProjectFile(uri.c_str(), *m_projectFile))
        return projectFile;
    projectFile.reset(m_projectFile);
    m_projectFile = NULL;
    return projectFile;
}

void ProjectFileCreatorDialog::validateInput()
{
    gtk_widget_set_sensitive(
        gtk_dialog_get_widget_for_response(m_dialog, GTK_RESPONSE_ACCEPT),
        m_projectFileEditor->inputValid());
}

}
