// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project.hpp"
#include "editor.hpp"
#include "file.hpp"
#include "../application.hpp"
#include "../resources/project-configuration.hpp"
#include <utility>
#include <map>

namespace Samoyed
{

Project::Project(const char *uri):
    m_uri(uri)
{
    Application::instance().addProject(*this);
}

Project::~Project()
{
    Application::instance().removeProject(*this);
    Application::instance().onProjectClosed();
}

bool Project::close()
{
    m_closing = true;
    if (!m_firstEditor)
    {
        delete this;
        return true;
    }

    // Close all editors.
    for (Editor *editor = m_firstEditor, *next; editor; editor = next)
    {
        next = editor->nextInProject();
        if (!editor->close())
        {
            m_closing = false;
            return false;
        }
    }
    return true;
}

Editor *Project::findEditor(const char *uri)
{
    EditorTable::const_iterator it = m_editorTable.find(uri);
    if (it == m_editorTable.end())
        return NULL;
    return it->second;
}

const Editor *Project::findEditor(const char *uri) const
{
    EditorTable::const_iterator it = m_editorTable.find(uri);
    if (it == m_editorTable.end())
        return NULL;
    return it->second;
}

void Project::addEditor(Editor &editor)
{
    m_editorTable.insert(std::make_pair(editor.file().uri(), &editor));
    editor.addToListInProject(m_firstEditor, m_lastEditor);
}

void Project::removeEditor(Editor &editor)
{
    std::pair<EditorTable::iterator, EditorTable::iterator> range =
        m_editorTable.equal_range(editor.file().uri());
    for (EditorTable::iterator it = range.first; it != range.second; ++it)
        if (it->second == &editor)
        {
            m_editorTable.erase(it);
            break;
        }
    editor.removeFromListInProject(m_firstEditor, m_lastEditor);
}

void Project::onEditorClosed()
{
    if (m_closing && !m_firstEditor)
        delete this;
}

}
