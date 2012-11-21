// Editor group.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_EDITOR_GROUP_HPP
#define SMYD_EDITOR_GROUP_HPP

#include <vector>
#include <boost/utility>
#include <gtk/gtk.h>

namespace Samoyed
{

class Editor;

/**
 * A editor group is actually a GTK+ notebook containing editors.
 */
class EditorGroup: public boost::noncopyable
{
public:
    int editorCount() const { return m_editors.size(); }

    Editor &editor(int i) const { return *m_editors[i]; }

    void addEditor(Editor &editor);

    void removeEditor(Editor &editor);

    int currentEditorIndex() const { return m_currentEditorIndex; }

    void setCurrentEditorIndex(int index) { m_currentEditorIndex = index; }

private:
    GtkNotebook *m_notebook;

    std::vector<Editor *> m_editors;

    int m_currentEditorIndex;
};

}

#endif
