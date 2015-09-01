// GNU build system per-file data.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "gnu-build-system-file.hpp"
#include "application.hpp"
#include <string.h>
#include <string>
#include <gtk/gtk.h>

namespace Samoyed
{

namespace GnuBuildSystem
{

GnuBuildSystemFile::Editor::Editor()
{
    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "plugins" G_DIR_SEPARATOR_S "gnu-build-system"
        G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "gnu-build-system-file-editors.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());
    m_instDirEntry = GTK_ENTRY(gtk_builder_get_object(m_builder,
                                                      "install-dir-entry"));
}

GnuBuildSystemFile::Editor::~Editor()
{
    g_object_unref(m_builder);
}

void GnuBuildSystemFile::Editor::addGtkWidgets(GtkGrid *grid)
{
    BuildSystemFile::Editor::addGtkWidgets(grid);
    GtkWidget *label = GTK_WIDGET(gtk_builder_get_object(m_builder,
                                                         "install-dir-label"));
    gtk_grid_attach_next_to(grid, label, NULL, GTK_POS_BOTTOM, 1, 1);
    gtk_grid_attach_next_to(grid, GTK_WIDGET(m_instDirEntry), label,
                            GTK_POS_RIGHT, 1, 1);
}

bool GnuBuildSystemFile::Editor::inputValid() const
{
    return BuildSystemFile::Editor::inputValid() &&
           *gtk_entry_get_text(m_instDirEntry);
}

void GnuBuildSystemFile::Editor::getInput(BuildSystemFile &file) const
{
    BuildSystemFile::Editor::getInput(file);
    GnuBuildSystemFile &f = static_cast<GnuBuildSystemFile &>(file);
    f.m_installDir = gtk_entry_get_text(m_instDirEntry);
}

GnuBuildSystemFile::~GnuBuildSystemFile()
{
}

bool GnuBuildSystemFile::read(const char *&data, int &dataLength)
{
    if (!BuildSystemFile::read(data, dataLength))
        return false;
    m_installDir = data;
    if (dataLength < m_installDir.length() + 1)
        return false;
    data += m_installDir.length() + 1;
    dataLength -= m_installDir.length() + 1;
    return true;
}

int GnuBuildSystemFile::dataLength() const
{
    return BuildSystemFile::dataLength() + m_installDir.length() + 1;
}

void GnuBuildSystemFile::write(char *&data) const
{
    BuildSystemFile::write(data);
    strcpy(data, m_installDir.c_str());
    data += m_installDir.length() + 1;
}

}

}
