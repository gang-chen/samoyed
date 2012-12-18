// Editor group.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "editor-group.hpp"
#include "editor.hpp"
#include <vector>
#include <gtk/gtk.h>

namespace Samoyed
{

void EditorGroup::addEditor(Editor &editor, int index)
{
    m_editors.insert(m_editors.begin() + index, &editor);
    editor.addToGroup(*this, index);
}

void EditorGroup::removeEditor(Editor &editor)
{
    m_editors.erase(m_editors.begin() + editor.index());
    editor.removeFromGroup();
    if (closing() && m_editors.empty())
        continueClosing();
}

bool EditorGroup::close()
{
    if (!(*(m_editors.begin() + m_currentIndex))->close())
        return false;
    for (std::vector<Editor *>::const_iterator it = m_editors.begin();
         it != m_editors.end();
         ++it)
    {
        if (!(*it)->closing() && !(*it)->close())
            return false;
    }
    closing() = true;
    if (m_editors.empty())
        delete this;
    return true;
}

}
