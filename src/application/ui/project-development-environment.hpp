// Project development environment.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PROJECT_DEVELOPMENT_ENVIRONMENT_HPP
#define SMYD_PROJECT_DEVELOPMENT_ENVIRONMENT_HPP

namespace Samoyed
{

class Project;
class ProjectExplorer;
class EditorGroup;
class FileEditor;

/**
 * A project development environment is a user interface where the user can
 * develop a project.  It contains a project explorer and one or more editor
 * groups.
 */
class ProjectDevelopmentEnvironment
{
public:
    /**
     * Retrieve an editor editing an opened file.
     */
    FileEditor *getFileEditor(const char *uri);

    /**
     * Open a file in a new editor, if the file is not opened.  Otherwise,
     * retrieve an existing editor for it or create a new editor if requested.
     * @param line The line number of the initial cursor, starting from 0.
     * @param column The column number of the initial cursor, the character
     * index, starting from 0.
     * @param newEditor True if request to always create a new editor.
     * @param editorContainer The editor group to contain the new editor, or
     * NULL if put the new editor into the current editor group.
     * @param editorIndex The index of the new editor in the containing editor
     * group, or -1 if put the new editor after the current editor.
     */
    FileEditor *openFile(const char *uri,
                         int line,
                         int column,
                         bool newEditor,
                         EditorGroup *editorGroup,
                         int editorIndex);

    bool closeEditor(Editor &editor);

private:
    Project *m_project;
    ProjectExplorer *m_projectExplorer;
    EditorGroup *m_editorGroups;
};

}

#endif
