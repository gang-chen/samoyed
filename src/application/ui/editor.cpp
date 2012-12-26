// File editor.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "editor.hpp"
#include "file.hpp"
#include "project.hpp"
#include "editor-group.hpp"

namespace Samoyed
{

Editor::Editor(File &file, Project &project):
    m_file(file),
    m_project(project),
    m_group(NULL),
    m_index(-1)
{
    m_project.addEditor(*this);
}

Editor::~Editor()
{
    assert(m_group);
    m_group->removeEditor(*this);
    m_project.removeEditor(*this);
    m_group->onEditorClosed();
}

bool Editor::close()
{
    return m_file.closeEditor(*this);
}

}
