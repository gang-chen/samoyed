// Actions definition and implementation.
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
    /**
     * The GTK+ actions that are always sensitive.
     */
    static const GtkActionEntry s_basicActionEntries[] =
    {
        // Top-level.
        { "session", NULL, N_("_Session") },
        { "project", NULL, N_("_Project") },
        { "edit", NULL, N_("_Edit") },
        { "view", NULL, N_("_View") },
        { "help", NULL, N_("_Help") },

        // Session menu.
        { "session-new", NULL, N_("_New..."), NULL,
          N_("Start a new session"), G_CALLBACK(newSession) },
        { "session-switch", NULL, N_("_Switch..."), NULL,
          N_("Switch to a saved session"), G_CALLBACK(switchSession) },
        { "session-manage", NULL, N_("_Manage..."), NULL,
          N_("Manage saved sessions"), G_CALLBACK(manageSessions) },
        { "session-quit", NULL, N_("_Quit"), "<control>q",
          N_("Quit the current session"), G_CALLBACK(quitSession) },

        // Project menu.
        { "project-new", NULL, N_("_New..."), NULL,
          N_("Create a project"), G_CALLBACK(newProject) },
        { "project-open", NULL, N_("_Open..."), NULL,
          N_("Open a project"), G_CALLBACK(openProject) },

        // Edit menu.
        { "edit-preferences", NULL, N_("_Preferences..."), NULL,
          N_("Edit preferences"), G_CALLBACK(editPreferences) },

        // View menu.
        { "view-new-window", NULL, N_("New _window"), NULL,
          N_("Create a window"), G_CALLBACK(newWindow) },
        { "view-new-subwindow", NULL, N_("New _subwindow"), NULL,
          N_("Create a subwindow"), G_CALLBACK(newSubwindow) },
        { "view-full-screen", NULL, N_("_Full screen"), NULL,
          N_("Enter full screen mode"), G_CALLBACK(enterFullScreen) },

        // Help menu.
        { "help-manual", NULL, N_("_Manual"), "F1",
          N_("Show the user manual"), G_CALLBACK(showManual) },
        { "help-tutorial", NULL, N_("_Tutorial"), NULL,
          N_("Show the tutorial"), G_CALLBACK(showTutorial) },
        { "help-about", NULL, N_("_About"), NULL,
          N_("About Samoyed", G_CALLBACK(showAbout) }
    };

    /**
     * The GTK+ actions that are sensitive when some projects are opened.
     */
    static const GtkActionEntry* s_actionEntriesForProjects[] =
    {
        // File menu.
    };

    /**
     * The GTK+ actions that are sensitive when some files are opened.
     */
    static const GtkActionEntry* s_actionEntriesForFiles[] =
    {
    };

private:
    static void newSession(GtkAction *action, Window *window);
    static void switchSession(GtkAction *action, Window *window);
    static void manageSessions(GtkAction *action, Window *window);
    static void quitSession(GtkAction *action, Window *window);

    static void newProject(GtkAction *action, Window *window);
    static void openProject(GtkAction *action, Window *window);

    static void editPreferences(GtkAction *action, Window *window);

    static void newWindow(GtkAction *action, Window *window);
    static void newSubwindow(GtkAction *action, Window *window);
    static void enterFullScreen(GtkAction *action, Window *window);

    static void showManual(GtkAction *action, Window *window);
    static void showTutorial(GtkAction *action, Window *window);
    static void showAbout(GtkAction *action, Window *window);
};

}

#endif
