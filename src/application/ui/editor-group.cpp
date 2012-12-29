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
    m_notebook = gtk_notebook_new();
}

EditorGroup::~EditorGroup()
{
    assert(m_editors.empty());
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
    GtkWidget *label = gtk_label_new(editor().title());
    gtk_widget_set_tooltip_text(label, editor.file().uri())
    gtk_notebook_insert_page(GTK_NOTEBOOK(m_notebook),
                             editor.gtkWidget(),
                             label,
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

void EditorGroup::onEditorTitleChanged(Editor &editor)
{
    GtkWidget *label = gtk_notebook_get_tab_label(GTK_NOTEBOOK(m_notebook),
                                                  editor.gtkWidget());
    gtk_label_set_text(GTK_LABEL(label), editor.title());
}

}
