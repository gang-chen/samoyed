// File editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_EDITOR_HPP
#define SMYD_EDITOR_HPP

#include "../utilities/misc.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

class File;
class Project;
class EditorGroup;
class File::EditPrimitive;

/**
 * An editor is used to edit an opened file in the context of an opend project.
 */
class Editor
{
public:
    Editor(File &file, Project &project);

    virtual ~Editor();

    virtual GtkWidget *gtkWidget() const;

    bool close();

    EditorGroup *editorGroup() const { return m_editorGroup; }
    int index() const { return m_index; }

    void addToGroup(EditorGroup &group, int index)
    {
        m_group = group;
        m_index = index;
    }

    void removeFromGroup()
    {
        m_group = NULL;
        m_index = -1;
    }

    void resetIndex(int index) { m_index = index; }

    virtual void onEdited(File::EditPrimitive &edit) = 0;

    virtual void onFileEditedStateChanged() = 0;

    virtual void freeze() = 0;

    virtual void unfreeze() = 0;

private:
    /**
     * The file being edited.
     */
    File &m_file;

    /**
     * The project where the editor is.
     */
    Project &m_project;

    EditorGroup *m_group;
    int m_index;

    bool m_closing;

    SMYD_DEFINE_DOUBLY_LINKED_IN(Editor, File)
    SMYD_DEFINE_DOUBLY_LINKED_IN(Editor, Project)
};

}

#endif
