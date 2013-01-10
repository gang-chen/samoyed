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
    Actions(Window *window);

    ~Actions();

    GtkActionGroup *actionGroup() const { return m_actionGroup; }

    void updateSensitivity(const Window *window);

private:
    enum
    {
        ,
    };

    static void createSession(GtkAction *action, Window *window);
    static void switchSession(GtkAction *action, Window *window);
    static void manageSessions(GtkAction *action, Window *window);
    static void quitSession(GtkAction *action, Window *window);

    static void createProject(GtkAction *action, Window *window);
    static void openProject(GtkAction *action, Window *window);
    static void closeProject(GtkAction *action, Window *window);
    static void closeAllProjects(GtkAction *action, Window *window);
    static void createFile(GtkAction *action, Window *window);
    static void createDirectory(GtkAction *action, Window *window);
    static void manageConfigurations(GtkAction *action, Window *window);

    static void openFile(GtkAction *action, Window *window);
    static void openFileInNewEditor(GtkAction *action, Window *window);
    static void saveFile(GtkAction *action, Window *window);
    static void saveAllFiles(GtkAction *action, Window *window);
    static void reloadFile(GtkAction *action, Window *window);
    static void closeFile(GtkAction *action, Window *window);
    static void closeAllFiles(GtkAction *action, Window *window);
    static void setupPage(GtkAction *action, Window *window);
    static void previewPrintedFile(GtkAction *action, Window *window);
    static void printFile(GtkAction *action, Window *window);

    static void undo(GtkAction *action, Window *window);
    static void redo(GtkAction *action, Window *window);
    static void cut(GtkAction *action, Window *window);
    static void copy(GtkAction *action, Window *window);
    static void paste(GtkAction *action, Window *window);
    static void deleteObject(GtkAction *action, Window *window);
    static void configure(GtkAction *action, Window *window);
    static void editPreferences(GtkAction *action, Window *window);

    static void createWindow(GtkAction *action, Window *window);
    static void createEditorGroup(GtkAction *action, Window *window);
    static void enterFullScreen(GtkAction *action, Window *window);

    static void showManual(GtkAction *action, Window *window);
    static void showTutorial(GtkAction *action, Window *window);
    static void showAbout(GtkAction *action, Window *window);

    static const GtkActionEntry s_actionEntries[] =
    {
        // Top-level.
        { "session", NULL, N_("_Session") },
        { "project", NULL, N_("_Project") },
        { "file", NULL, N_("_File") },
        { "edit", NULL, N_("_Edit") },
        { "search", NULL, N_("_Search") },
        { "view", NULL, N_("_View") },
        { "help", NULL, N_("_Help") },

        // Session menu.
        { "session-new", NULL, N_("_New..."), NULL,
          N_("Quit the current session and start a new one"),
          G_CALLBACK(createSession) },
        { "session-switch", NULL, N_("_Switch..."), NULL,
          N_("Quit the current session and restore a saved one"),
          G_CALLBACK(switchSession) },
        { "session-manage", NULL, N_("_Manage"), NULL,
          N_("Manage saved sessions"), G_CALLBACK(manageSessions) },
        { "session-quit", NULL, N_("_Quit"), "<control>Q",
          N_("Quit the current session"), G_CALLBACK(quitSession) },

        // Project menu.
        { "project-new", NULL, N_("_New..."), NULL,
          N_("Create a project"), G_CALLBACK(createProject) },
        { "project-open", NULL, N_("_Open..."), NULL,
          N_("Open a project"), G_CALLBACK(openProject) },
        { "project-close", NULL, N_("_Close"), NULL,
          N_("Close the current project"), G_CALLBACK(closeProject) },
        { "project-close-all", NULL, N_("Close _All"), NULL,
          N_("Close all opened projects"), G_CALLBACK(closeAllProjects) },
        { "project-new-file", NULL, N_("New _File..."), NULL,
          N_("Create a file"), G_CALLBACK(createFile) },
        { "project-new-directory", NULL, N_("New _Directory..."), NULL,
          N_("Create a directory"), G_CALLBACK(createDirectory) },
        { "configure", NULL, N_("Confi_gure"), NULL,
          N_("Configure the selected object"), G_CALLBACK(configure) },
        { "project-manage-configurations", NULL,
          N_("Manage Confi_gurations"), NULL,
          N_("Manage configurations of the current project"),
          G_CALLBACK(manageConfigurations) },

        // File menu.
        { "file-open", NULL, N_("_Open"), NULL,
          N_("Open the selected file"), G_CALLBACK(openFile) },
        { "file-open-new-editor", NULL, N_("Open in New _Editor"), NULL,
          N_("Open the selected file in a new editor"),
          G_CALLBACK(openFileInNewEditor) },
        { "file-save", NULL, N_("_Save"), NULL,
          N_("Save the current file"), G_CALLBACK(saveFile) },
        { "file-save-all", NULL, N_("Save _All"), NULL,
          N_("Save all edited files"), G_CALLBACK(saveAllFiles) },
        { "file-reload", NULL, N_("_Reload"), NULL,
          N_("Reload the curren file"), G_CALLBACK(reloadFile) },
        { "file-close", NULL, N_("_Close"), NULL,
          N_("Close the current file"), G_CALLBACK(closeFile) },
        { "file-close-all", NULL, N_("C_lose All"), NULL,
          N_("Close all opened files"), G_CALLBACK(closeAllFiles) },
        { "file-page-setup", NULL, N_("Page _Setup..."), NULL,
          N_("Set up the page settings for printing"), G_CALLBACK(setupPage) },
        { "file-print-preview", NULL, N_("Print Pre_view"), NULL,
          N_("Preview the printed file"), G_CALLBACK(previewPrintedFile) },
        { "file-print", NULL, N_("_Print"), NULL,
          N_("Print the current file"), G_CALLBACK(printFile) },

        // Edit menu.
        { "edit-undo", NULL, N_("_Undo"), "<control>Z",
          N_("Undo the last operation"), G_CALLBACK(undo) },
        { "edit-redo", NULL, N_("_Redo"), "<shift><control>Z",
          N_("Redo the last undone operation"), G_CALLBACK(redo) },
        { "edit-cut", NULL, N_("Cu_t"), "<control>X",
          N_("Cut the selected object"), G_CALLBACK(cut) },
        { "edit-copy", NULL, N_("_Copy"), "<control>C",
          N_("Copy the selected object"), G_CALLBACK(copy) },
        { "edit-paste", NULL, N_("_Paste"), "<control>V",
          N_("Paste the object in the clipboard"), G_CALLBACK(paste) },
        { "edit-delete", NULL, N_("_Delete"), NULL,
          N_("Delete the selected object"), G_CALLBACK(deleteObject) },
        { "edit-preferences", NULL, N_("Pre_ferences"), NULL,
          N_("Edit your preferences"), G_CALLBACK(editPreferences) },

        // View menu.
        { "view-new-window", NULL, N_("New _Window"), NULL,
          N_("Create a window"), G_CALLBACK(createWindow) },
        { "view-new-editor-group", NULL, N_("New Editor _Group"), NULL,
          N_("Create an editor group"), G_CALLBACK(createEditorGroup) },
        { "view-full-screen", NULL, N_("_Full Screen"), NULL,
          N_("Enter full screen mode"), G_CALLBACK(enterFullScreen) },

        // Help menu.
        { "help-manual", NULL, N_("_Manual"), "F1",
          N_("Show the user manual"), G_CALLBACK(showManual) },
        { "help-tutorial", NULL, N_("_Tutorial"), NULL,
          N_("Show the tutorial"), G_CALLBACK(showTutorial) },
        { "help-about", NULL, N_("_About"), NULL,
          N_("About Samoyed", G_CALLBACK(showAbout) }

        // New popup menu.
        { "new-file", NULL, N_("_File"), NULL,
          N_("Create a file"), G_CALLBACK(createFile) },
        { "new-directory", NULL, N_("_Directory"), NULL,
          N_("Create a directory"), G_CALLBACK(createDirectory) }
    };

//// init actions, add to action group
    GtkActionGroup *m_actionGroup;
    GtkAction *m_actions[];
};

}

#endif
