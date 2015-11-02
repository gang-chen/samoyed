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
        ACTION_SESSION,
        ACTION_PROJECT,
        ACTION_FILE,
        ACTION_EDIT,
        ACTION_SEARCH,
        ACTION_BUILD,
        ACTION_VIEW,
        ACTION_HELP,

        ACTION_CREATE_SESSION,
        ACTION_SWITCH_SESSION,
        ACTION_EDIT_PREFERENCES,
        ACTION_MANAGE_SESSIONS,
        ACTION_QUIT_SESSION,

        ACTION_CREATE_PROJECT,
        ACTION_OPEN_PROJECT,
        ACTION_CLOSE_PROJECT,
        ACTION_CLOSE_ALL_PROJECTS,
        ACTION_CREATE_DIRECTORY,
        ACTION_CREATE_SOURCE_FILE,
        ACTION_CREATE_HEADER_FILE,
        ACTION_CREATE_FILE,
        ACTION_CREATE_STATIC_LIBRARY,
        ACTION_CREATE_SHARED_LIBRARY,
        ACTION_CREATE_PROGRAM,
        ACTION_CREATE_TARGET,
        ACTION_EDIT_PROPERTIES,

        ACTION_OPEN_FILE,
        ACTION_SAVE_FILE,
        ACTION_SAVE_ALL_FILES,
        ACTION_RELOAD_FILE,
        ACTION_CLOSE_FILE,
        ACTION_CLOSE_ALL_FILES,

        ACTION_UNDO,
        ACTION_REDO,
        ACTION_CUT,
        ACTION_COPY,
        ACTION_PASTE,
        ACTION_DELETE,
        ACTION_INDENT,

        ACTION_CONFIGURE_PROJECT,
        ACTION_BUILD_PROJECT,
        ACTION_INSTALL_PROJECT,
        ACTION_CLEAN_PROJECT,
        ACTION_CREATE_CONFIGURATION,
        ACTION_SET_ACTIVE_CONFIGURATION,
        ACTION_MANAGE_CONFIGURATIONS,

        ACTION_CREATE_WINDOW,
        ACTION_WINDOW_LAYOUT,
        ACTION_CREATE_EDITOR,
        ACTION_MOVE_EDITOR_DOWN,
        ACTION_MOVE_EDITOR_RIGHT,
        ACTION_SPLIT_EDITOR_HORIZONTALLY,
        ACTION_SPLIT_EDITOR_VERTICALLY,

        ACTION_SHOW_MANUAL,
        ACTION_SHOW_TUTORIAL,
        ACTION_SHOW_ABOUT,

        N_ACTIONS
    };

    enum ToggleActionIndex
    {
        TOGGLE_ACTION_SHOW_HIDE_TOOLBAR,
        TOGGLE_ACTION_SHOW_HIDE_STATUS_BAR,
        TOGGLE_ACTION_ENTER_LEAVE_FULL_SCREEN,

        N_TOGGLE_ACTIONS
    };

    enum RadioActionGroupIndex
    {
        RADIO_ACTION_GROUP_WINDOW_LAYOUT,

        N_RADIO_ACTION_GROUPS
    };

    static void invalidateSensitivity();

    Actions(Window *window);
    ~Actions();

    GtkAction *action(ActionIndex index) const { return m_actions[index]; }
    GtkToggleAction *toggleAction(ToggleActionIndex index) const
    { return m_toggleActions[index]; }
    GtkRadioAction *radioActionGroup(RadioActionGroupIndex index) const
    { return m_radioActionGroups[index]; }

    GtkActionGroup *actionGroup() const { return m_actionGroup; }

    void onToolbarVisibilityChanged(bool visible);
    void onStatusBarVisibilityChanged(bool visible);
    void onWindowFullScreenChanged(bool inFullScreen);

    void updateStatefulActions();

private:
    static gboolean updateSensitivity(gpointer);

    static bool s_sensitivityUpdaterAdded;

    Window *m_window;

    GtkAction *m_actions[N_ACTIONS];
    GtkToggleAction *m_toggleActions[N_TOGGLE_ACTIONS];
    GtkRadioAction *m_radioActionGroups[N_RADIO_ACTION_GROUPS];

    GtkActionGroup *m_actionGroup;
};

}

#endif
