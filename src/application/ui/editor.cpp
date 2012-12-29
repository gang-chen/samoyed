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
    m_index(-1),
    m_closing(false)
{
    m_project.addEditor(*this);
    if (file.edited())
        m_title = "*";
    m_title += file.name();
}

Editor::~Editor()
{
    assert(m_group);
    assert(m_closing);
    m_group->removeEditor(*this);
    m_project.removeEditor(*this);
    m_group->onEditorClosed();
}

bool Editor::close()
{
    // It is possible the editor is requested to be closed for multiple times.
    if (m_closing)
        return true;
    m_closing = true;
    if (!m_file.closeEditor(*this))
    {
        m_closing = false;
        return false;
    }
    return true;
}

void Editor::onFileEditedStateChanged()
{
    if (m_file.edited())
        m_title = "*";
    else
        m_title.clear();
    m_title += m_file.name();
    m_group->onEditorTitleChanged(*this);
}

}
