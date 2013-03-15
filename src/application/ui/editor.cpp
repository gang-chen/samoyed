// File editor.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "editor.hpp"
#include "file.hpp"
#include "project.hpp"

namespace Samoyed
{

Editor::Editor(File &file, Project *project):
    m_file(file),
    m_project(project)
{
    if (m_project)
        m_project->addEditor(*this);
}

Editor::~Editor()
{
    if (m_project)
        m_project->removeEditor(*this);
}

bool Editor::close()
{
    if (closing())
        return true;

    setClosing(true);
    if (!m_file.closeEditor(*this))
    {
        setClosing(false);
        return false;
    }
    return true;
}

void Editor::onEditedStateChanged()
{
}

}
