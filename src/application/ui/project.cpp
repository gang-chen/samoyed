// Opened project.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "project.hpp"
#include "editor.hpp"
#include "file.hpp"
#include "project-explorer.hpp"
#include "../application.hpp"
#include <utility>
#include <map>

namespace Samoyed
{

Project::~Project()
{
    Application::instance().projectExplorer().removeProject(*this);
}

bool Project::close()
{
    // Close all editors.
    for (Editor *editor = m_firstEditor, *next; editor; editor = next)
    {
        next = editor->nextInProject();
        if (!editor->close())
            return false;
    }
    m_closing = true;
    if (!m_firstEditor)
        delete this;
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
    m_editorTable.erase(editor.file().uri());///// multi_map...
    editor.removeFromListInProject(m_firstEditor, m_lastEditor);
    if (m_closing && !m_firstEditor)
        continueClosing();
}

}
