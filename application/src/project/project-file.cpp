// Project-managed file.
// Copyright (C) 2015 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project-file.hpp"
#include "project.hpp"
#include "build-system/build-system.hpp"
#include "build-system/build-system-file.hpp"
#include <stdlib.h>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_array.hpp>

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
    if (dataLength < sizeof(int))
        return NULL;
    int type = *reinterpret_cast<const int *>(data);
    data += sizeof(int);
    dataLength -= sizeof(int);
    ProjectFile *file = project.createProjectFile(type);
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
        sizeof(int) + this->dataLength() + buildSystemData().dataLength();
    char *d = new char[dataLength];
    data.reset(d);
    *reinterpret_cast<int *>(d) = m_type;
    d += sizeof(int);
    writeInternally(d);
    buildSystemData().write(d);
}

ProjectFile::Editor *ProjectFile::createEditor() const
{
    BuildSystemFile::Editor *buildSystemDataEditor =
        m_buildSystemData->createEditor();
    return new Editor(buildSystemDataEditor);
}

}
