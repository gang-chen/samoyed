// Editor group.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "editor-group.hpp"
#include "editor.hpp"

namespace Samoyed
{

void EditorGroup::addEditor(Editor &editor)
{
    int index = m_editors.size();
    m_editors.push_back(&editor);
    editor.addToGroup(*this, index);
}

void EditorGroup::removeEditor(Editor &editor)
{
    m_editors.erase(m_editors.begin + editor.index());
    editor.removeFromGroup();
}

}
