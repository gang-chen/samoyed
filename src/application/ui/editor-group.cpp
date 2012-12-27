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

EditorGroup::EditorGroup()
{
}

EditorGroup::~EditorGroup()
{
    gtk_widget_destroy(m_notebook);
}

bool EditorGroup::close()
{
    setClosing(true);
    if (m_editors.empty())
    {
        delete this;
        return true;
    }

    // First close the current editor and then close the others.
    int currentIndex = currentEditorIndex();
    std::vector<Editor *> editors(m_editors);
    if (!m_editors[currentIndex]->close())
    {
        setClosing(false);
        return false;
    }
    for (int i = 0; i < editors.size(); i++)
    {
        if (i != currentIndex)
            if (!editors[i]->close())
            {
                setClosing(false);
                return false;
            }
    }
    return true;
}

void EditorGroup::addEditor(Editor &editor, int index)
{
    m_editors.insert(m_editors.begin() + index, &editor);
    for (int i = index + 1; i < m_editors.size(); i++)
        m_editors[i]->setIndex(i);
    editor.addToGroup(*this, index);
    gtk_notebook_insert_page(GTK_NOTEBOOK(m_notebook),
                             editor.gtkWidget(),
                             editor(),
                             index);
}

void EditorGroup::removeEditor(Editor &editor)
{
    m_editors.erase(m_editors.begin() + editor.index());
    for (int i = editor.index(); i < m_editors.size(); i++)
        m_editors[i]->setIndex(i);
    editor.removeFromGroup();
}

void EditorGroup::onEditorClosed()
{
    if (closing() && m_editors.empty())
        delete this;
}

}
