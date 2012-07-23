// Actions definition and implementation.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_ACTIONS_HPP
#define SMYD_ACTIONS_HPP

namespace Samoyed
{

/**
 * This is a helper class for class Window.  It defines and implements actions.
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
        { "session", 0, N_("_Session") },
        { "project", 0, N_("_Project") },
        { "edit", 0, N_("_Edit") },
        { "view", 0, N_("_View") },
        { "help", 0, N_("_Help") },

        // Session menu.
        { "session-new", 0, N_("_New..."), 0,
          N_("Start a new session"), G_CALLBACK(newSession) },
        { "session-switch", 0, N_("_Switch..."), 0,
          N_("Switch to a saved session"), G_CALLBACK(switchSession) },
        { "session-manage", 0, N_("_Manage..."), 0,
          N_("Manage saved sessions"), G_CALLBACK(manageSessions) },
        { "session-quit", 0, N_("_Quit"), "<control>q",
          N_("Quit the current session"), G_CALLBACK(quitSession) },

        // Project menu.
        { "project-new", 0, N_("_New..."), 0,
          N_("Create a new project"), G_CALLBACK(newProject) },

        // Help menu.
        { "help-manual", 0, N_("_Manual"), "F1",
          N_("Show the user manual"), G_CALLBACK(showManual) },
        { "help-tutorial", 0, N_("_Tutorial"), 0,
          N_("Show the tutorial"), G_CALLBACK(showTutorial) },
        { "help-about", 0, N_("_About"), 0,
          N_("About this application", G_CALLBACK(showAbout) }
    };

    /**
     * The GTK+ actions that are sensitive when some projects are opened.
     */
    static const GtkActionEntry* s_projectActionEntries[] =
    {
    };

    /**
     * The GTK+ actions that are sensitive when some files are opened.
     */
    static const GtkActionEntry* s_documentActionEntries[] =
    {
    };

private:
    static void newSession(gpointer window);

    static void switchSession(gpointer window);

    static void manageSession(gpointer window);

    static void quitSession(gpointer window);
};

}

#endif
