// Editor.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_EDITOR_HPP
#define SMYD_EDITOR_HPP

namespace Samoyed
{

class Project;
class File;
class EditorGroup;

/**
 * An editor is used to edit an opened file in the context of an opend project.
 */
class Editor
{
public:
    Editor(Project &project, File &file);

    virtual ~Editor();

    void addToListInProject(Editor *&editors);
    void removeFromListInProject(Editor *&editors);

    Editor *nextInProject() const { return m_nextInProject; }
    Editor *previousInProject() const { return m_previousInProject; }

    void addToListInFile(Editor *&editors);
    void removeFromListInFile(Editor *&editors);

    Editor *nextInFile() const { return m_nextInFile; }
    Editor *previousInFile() const { return m_previousInFile; }

    void addToGroup(EditorGroup &group, int index);
    void removeFromGroup();

private:
    /**
     * The project where the editor is.
     */
    Project &m_project;

    Editor *m_nextInProject;
    Editor *m_prevInProject;

    /**
     * The file being edited.
     */
    File &m_file;

    Editor *m_nextInFile;
    Editor *m_prevInFile;

    EditorGroup *m_group;
    int m_index;
};

}

#endif
