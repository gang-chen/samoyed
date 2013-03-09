// File editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_EDITOR_HPP
#define SMYD_EDITOR_HPP

#include "widget.hpp"
#include "file.hpp"
#include "../utilities/miscellaneous.hpp"
#include <string>
#include <gtk/gtk.h>

namespace Samoyed
{

class Project;

/**
 * An editor is used to edit an opened file in the context of an opend project.
 */
class Editor: public Widget
{
public:
    Editor(File &file, Project *project);

    virtual ~Editor();

    File &file() { return m_file; }
    const File &file() const { return m_file; }

    virtual bool close();

    virtual void onEdited(const File::EditPrimitive &edit) = 0;

    virtual void onEditedStateChanged();

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
    Project *m_project;

    SAMOYED_DEFINE_DOUBLY_LINKED_IN(Editor, File)
    SAMOYED_DEFINE_DOUBLY_LINKED_IN(Editor, Project)
};

}

#endif
