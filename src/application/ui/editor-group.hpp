// Editor group.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_EDITOR_GROUP_HPP
#define SMYD_EDITOR_GROUP_HPP

#include "pane.hpp"
#include <vector>
#include <gtk/gtk.h>

namespace Samoyed
{

class Editor;

/**
 * A editor group is actually a GTK+ notebook containing editors.
 */
class EditorGroup: public Pane
{
public:
    EditorGroup();

    virtual bool close();

    virtual GtkWidget *gtkWidget() const { return m_notebook; }

    int editorCount() const { return m_editors.size(); }

    Editor &editor(int i) const { return *m_editors[i]; }

    void addEditor(Editor &editor, int index);

    void removeEditor(Editor &editor);

    void onEditorClosed();

    int currentEditorIndex() const
    { return gtk_notebook_get_current_page(GTK_NOTEBOOK(m_notebook)); }

    void setCurrentEditorIndex(int index)
    { gtk_notebook_set_current_page(GTK_NOTEBOOK(m_notebook), index); }

    void onEditorEditedStateChanged(Editor &editor);

protected:
    virtual ~EditorGroup();

private:
    static void onCloseButtonClicked(GtkButton *button, gpointer editor);

    std::vector<Editor *> m_editors;
    std::vector<GtkWidget *> m_titles;

    GtkWidget *m_notebook;
};

}

#endif
