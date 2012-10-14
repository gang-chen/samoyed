// Editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_EDITOR_HPP
#define SMYD_EDITOR_HPP

namespace Samoyed
{

class File;
class Project;
class EditorGroup;

/**
 * An editor is used to edit an opened file by the user.  An editor can be
 * created for a file only in the context of a project.  Then the editor is
 * related to the project and its behavior is affected by the project.  For
 * instance, it may display the information specific to the project.  And when
 * the project is closed, the editor will also be destroyed.  In this sense, the
 * lifetime of the editor is managed by the related project.
 */
class Editor
{
private:
    /**
     * The file being edited.
     */
    File *m_file;

    Editor *m_nextInFile;
    Editor *m_prevInFile;

    /**
     * The related project.
     */
    Project *m_project;

    Editor *m_nextInProject;
    Editor *m_prevInProject;

    EditorGroup *m_group;
    int m_index;

    friend class Project;
    friend class File;
};

}

#endif
