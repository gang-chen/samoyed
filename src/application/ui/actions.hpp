// Actions.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_ACTIONS_HPP
#define SMYD_ACTIONS_HPP

#include <gtk/gtk.h>

namespace Samoyed
{

class Window;
class Editor;
class File;

/**
 * This class defines and implements actions.
 */
class Actions
{
public:
    enum ActionIndex
    {
        ACTION_SESSION,
        ACTION_PROJECT,
        ACTION_FILE,
        ACTION_EDIT,
        ACTION_SEARCH,
        ACTION_VIEW,
        ACTION_HELP,

        ACTION_CREATE_SESSION,
        ACTION_SWITCH_SESSION,
        ACTION_MANAGE_SESSIONS,
        ACTION_QUIT_SESSION,

        ACTION_CREATE_PROJECT,
        ACTION_OPEN_PROJECT,
        ACTION_CLOSE_PROJECT,
        ACTION_CLOSE_ALL_PROJECTS,
        ACTION_CREATE_FILE,
        ACTION_CREATE_DIRECTORY,
        ACTION_CONFIGURE,
        ACTION_MANAGE_CONFIGURATIONS,

        ACTION_OPEN_FILE,
        ACTION_SAVE_FILE,
        ACTION_SAVE_ALL_FILES,
        ACTION_RELOAD_FILE,
        ACTION_CLOSE_FILE,
        ACTION_CLOSE_ALL_FILES,
        ACTION_SETUP_PAGE,
        ACTION_PREVIEW_PRINTED_FILE,
        ACTION_PRINT_FILE,

        ACTION_UNDO,
        ACTION_REDO,
        ACTION_CUT,
        ACTION_COPY,
        ACTION_PASTE,
        ACTION_DELETE,
        ACTION_SELECT_ALL,
        ACTION_EDIT_PREFERENCES,

        ACTION_CREATE_WINDOW,
        ACTION_CREATE_EDITOR_GROUP,
        ACTION_CREATE_EDITOR,

        ACTION_SHOW_MANUAL,
        ACTION_SHOW_TUTORIAL,
        ACTION_SHOW_ABOUT,

        N_ACTIONS
    };

    enum ToggleActionIndex
    {
        TOGGLE_ACTION_ENTER_LEAVE_FULL_SCREEN,
        TOGGLE_ACTION_SHOW_HIDE_TOOLBAR,

        N_TOGGLE_ACTIONS
    };

    Actions(Window *window);

    ~Actions();

    GtkAction *action(ActionIndex index) const { return m_actions[index]; }
    GtkToggleAction *toggleAction(ToggleActionIndex index) const
    { return m_toggleActions[index]; }

    GtkActionGroup *actionGroup() const { return m_actionGroup; }

    void onEditorAdded(const Editor &editor);
    void onEditorClosed(const Editor &editor);
    void onFileChanged(const File &file);

private:
    Window *m_window;

    GtkAction *m_actions[N_ACTIONS];
    GtkToggleAction *m_toggleActions[N_TOGGLE_ACTIONS];

    GtkActionGroup *m_actionGroup;
};

}

#endif
