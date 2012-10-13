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
 * An editor is used to edit an opened file by the user.  An editor is related
 * to a project.  It may display the information specific to the project.  And
 * when the project is closed, the editor will also be closed.
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
};

}

#endif
