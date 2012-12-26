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

    virtual ~EditorGroup();

    virtual bool close();

    virtual GtkWidget *gtkWidget() const { return m_notebook; }

    int editorCount() const { return m_editors.size(); }

    Editor &editor(int i) const { return *m_editors[i]; }

    void addEditor(Editor &editor);

    void removeEditor(Editor &editor);

    void onEditorClosed();

    int currentEditorIndex() const;

    void setCurrentEditorIndex(int index);

private:
    std::vector<Editor *> m_editors;

    GtkWidget *m_notebook;
};

}

#endif
