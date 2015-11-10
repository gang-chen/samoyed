// Project-managed file.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-file.hpp"
#include "project.hpp"
#include "build-system/build-system.hpp"
#include "build-system/build-system-file.hpp"
#include "window/window.hpp"
#include "application.hpp"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_array.hpp>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

namespace
{

const char *TYPE_DESCRIPTIONS[Samoyed::ProjectFile::N_TYPES] =
{
    "directory",
    "source file",
    "header file",
    "file",
    "static library",
    "shared library",
    "program",
    "target"
};

}

namespace Samoyed
{

ProjectFile::Editor::Editor(BuildSystemFile::Editor *buildSystemDataEditor):
    m_buildSystemDataEditor(buildSystemDataEditor)
{
    m_buildSystemDataEditor->setChangedCallback(boost::bind(onChanged, this));
}

ProjectFile::Editor::~Editor()
{
    delete m_buildSystemDataEditor;
}

void ProjectFile::Editor::addGtkWidgets(GtkGrid *grid)
{
    addGtkWidgetsInternally(grid);
    m_buildSystemDataEditor->addGtkWidgets(grid);
}

bool ProjectFile::Editor::inputValid() const
{
    return inputValidInternally() && m_buildSystemDataEditor->inputValid();
}

void ProjectFile::Editor::getInput(ProjectFile &file) const
{
    getInputInternally(file);
    m_buildSystemDataEditor->getInput(file.buildSystemData());
}

ProjectFile::~ProjectFile()
{
    delete m_buildSystemData;
}

ProjectFile *ProjectFile::read(const Project &project,
                               const char *data,
                               int dataLength)
{
    if (static_cast<size_t>(dataLength) < sizeof(gint32))
        return NULL;
    gint32 type;
    memcpy(&type, data, sizeof(gint32));
    data += sizeof(gint32);
    dataLength -= sizeof(gint32);
    type = GINT32_FROM_LE(type);
    ProjectFile *file = project.createFile(type);
    if (!file ||
        !file->readInternally(data, dataLength) ||
        !file->buildSystemData().read(data, dataLength))
    {
        delete file;
        return NULL;
    }
    return file;
}

void ProjectFile::write(boost::shared_array<char> &data, int &dataLength) const
{
    dataLength =
        sizeof(gint32) + this->dataLength() + buildSystemData().dataLength();
    char *d = new char[dataLength];
    data.reset(d);
    gint32 type = GINT32_TO_LE(m_type);
    memcpy(d, &type, sizeof(gint32));
    d += sizeof(gint32);
    writeInternally(d);
    buildSystemData().write(d);
}

bool ProjectFile::createInStorage(const char *uri) const
{
    char *name = g_filename_from_uri(uri, NULL, NULL);
    int error;
    if (m_type == TYPE_DIRECTORY)
        error = g_mkdir(name, 0755);
    else if (m_type == TYPE_SOURCE_FILE ||
             m_type == TYPE_HEADER_FILE ||
             m_type == TYPE_GENERIC_FILE)
    {
        int fd = g_open(name, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd == -1)
            error = -1;
        else
            close(fd);
    }
    else
        error = 0;
    g_free(name);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create %s \"%s\"."),
            typeDescription(), uri);
        gtkMessageDialogAddDetails(
            dialog,
            _("%s."),
            g_strerror(errno));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }
    return true;
}

bool ProjectFile::removeFromStorage(const char *uri) const
{
    char *name = g_filename_from_uri(uri, NULL, NULL);
    int error;
    if (m_type == TYPE_DIRECTORY)
        error = g_rmdir(name);
    else if (m_type == TYPE_SOURCE_FILE ||
             m_type == TYPE_HEADER_FILE ||
             m_type == TYPE_GENERIC_FILE)
        error = g_unlink(name);
    else
        error = 0;
    g_free(name);
    if (error)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
            NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to create %s \"%s\"."),
            typeDescription(), uri);
        gtkMessageDialogAddDetails(
            dialog,
            _("%s."),
            g_strerror(errno));
        gtk_dialog_set_default_response(GTK_DIALOG(dialog),
            GTK_RESPONSE_CLOSE);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return false;
    }
    return true;
}

ProjectFile::Editor *ProjectFile::createEditor() const
{
    BuildSystemFile::Editor *buildSystemDataEditor =
        m_buildSystemData->createEditor();
    return new Editor(buildSystemDataEditor);
}

const char *ProjectFile::typeDescription() const
{
    return TYPE_DESCRIPTIONS[m_type];
}

}
