// Actions.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_ACTIONS_HPP
#define SMYD_ACTIONS_HPP

#include <gtk/gtk.h>

namespace Samoyed
{

class Window;

/**
 * This class defines and implements actions.
 */
class Actions
{
public:
    enum ActionIndex
    {
        SESSION,
        PROJECT,
        FILE,
        EDIT,
        SEARCH,
        VIEW,
        HELP,

        CREATE_SESSION,
        SWITCH_SESSION,
        MANAGE_SESSIONS,
        QUIT_SESSION,

        CREATE_PROJECT,
        OPEN_PROJECT,
        CLOSE_PROJECT,
        CLOSE_ALL_PROJECTS,
        CREATE_FILE,
        CREATE_DIRECTORY,
        CONFIGURE,
        MANAGE_CONFIGURATIONS,

        OPEN_FILE,
        OPEN_FILE_IN_NEW_EDITOR,
        SAVE_FILE,
        SAVE_ALL_FILES,
        RELOAD_FILE,
        CLOSE_FILE,
        CLOSE_ALL_FILES,
        SETUP_PAGE,
        PREVIEW_PRINTED_FILE,
        PRINT_FILE,

        UNDO,
        REDO,
        CUT,
        COPY,
        PASTE,
        DELETE,
        EDIT_PREFERENCES,

        CREATE_WINDOW,
        CREATE_EDITOR_GROUP,
        ENTER_FULL_SCREEN,

        SHOW_MANUAL,
        SHOW_TUTORIAL,
        SHOW_ABOUT,

        N_ACTIONS
    };

    Actions(Window *window);

    ~Actions();

    GtkActionGroup *actionGroup() const { return m_actionGroup; }

    void updateSensitivity(const Window *window);

private:
    GtkActionGroup *m_actionGroup;

    GtkAction *m_actions[N_ACTIONS];
};

}

#endif
