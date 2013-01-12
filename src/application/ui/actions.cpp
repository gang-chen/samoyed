// Actions.
// Copyright (C) 2012 Gang Chen.

#include "actions.hpp"
#include "window.hpp"
#include <gtk/gtk.h>

namespace
{

void createSession(GtkAction *action, Window *window)
{
}

void switchSession(GtkAction *action, Window *window)
{
}

void manageSessions(GtkAction *action, Window *window)
{
}

void quitSession(GtkAction *action, Window *window)
{
}

void createProject(GtkAction *action, Window *window)
{
}

void openProject(GtkAction *action, Window *window)
{
}

void closeProject(GtkAction *action, Window *window)
{
}

void closeAllProjects(GtkAction *action, Window *window)
{
}

void createFile(GtkAction *action, Window *window)
{
}

void createDirectory(GtkAction *action, Window *window)
{
}

void configure(GtkAction *action, Window *window)
{
}

void manageConfigurations(GtkAction *action, Window *window)
{
}

void openFile(GtkAction *action, Window *window)
{
}

void openFileInNewEditor(GtkAction *action, Window *window)
{
}

void saveFile(GtkAction *action, Window *window)
{
}

void saveAllFiles(GtkAction *action, Window *window)
{
}

void reloadFile(GtkAction *action, Window *window)
{
}

void closeFile(GtkAction *action, Window *window)
{
}

void closeAllFiles(GtkAction *action, Window *window)
{
}

void setupPage(GtkAction *action, Window *window)
{
}

void previewPrintedFile(GtkAction *action, Window *window)
{
}

void printFile(GtkAction *action, Window *window)
{
}

void undo(GtkAction *action, Window *window)
{
}

void redo(GtkAction *action, Window *window)
{
}

void cut(GtkAction *action, Window *window)
{
}

void copy(GtkAction *action, Window *window)
{
}

void paste(GtkAction *action, Window *window)
{
}

void deleteObject(GtkAction *action, Window *window)
{
}

void editPreferences(GtkAction *action, Window *window)
{
}

void createWindow(GtkAction *action, Window *window)
{
}

void createEditorGroup(GtkAction *action, Window *window)
{
}

void enterFullScreen(GtkAction *action, Window *window)
{
}

void showManual(GtkAction *action, Window *window)
{
}

void showTutorial(GtkAction *action, Window *window)
{
}

void showAbout(GtkAction *action, Window *window)
{
}

const GtkActionEntry actionEntries[Samoyed::Actions::N_ACTIONS] =
{
    // Top-level.
    { "session", NULL, N_("_Session"), NULL, NULL, NULL },
    { "project", NULL, N_("_Project"), NULL, NULL, NULL },
    { "file", NULL, N_("_File"), NULL, NULL, NULL },
    { "edit", NULL, N_("_Edit"), NULL, NULL, NULL },
    { "search", NULL, N_("_Search"), NULL, NULL, NULL },
    { "view", NULL, N_("_View"), NULL, NULL, NULL },
    { "help", NULL, N_("_Help"), NULL, NULL, NULL },

    // Session menu.
    { "session-new", NULL, N_("_New..."), NULL,
      N_("Quit the current session and start a new one"),
      G_CALLBACK(createSession) },
    { "session-switch", NULL, N_("_Switch..."), NULL,
      N_("Quit the current session and restore a saved one"),
      G_CALLBACK(switchSession) },
    { "session-manage", NULL, N_("_Manage"), NULL,
      N_("Manage saved sessions"), G_CALLBACK(manageSessions) },
    { "session-quit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",
      N_("Quit the current session"), G_CALLBACK(quitSession) },

    // Project menu.
    { "project-new", GTK_STOCK_NEW, N_("_New..."), NULL,
      N_("Create a project"), G_CALLBACK(createProject) },
    { "project-open", GTK_STOCK_OPEN, N_("_Open..."), NULL,
      N_("Open a project"), G_CALLBACK(openProject) },
    { "project-close", GTK_STOCK_CLOSE, N_("_Close"), NULL,
      N_("Close the current project"), G_CALLBACK(closeProject) },
    { "project-close-all", NULL, N_("Close _All"), NULL,
      N_("Close all opened projects"), G_CALLBACK(closeAllProjects) },
    { "project-new-file", GTK_STOCK_FILE, N_("New _File..."), NULL,
      N_("Create a file"), G_CALLBACK(createFile) },
    { "project-new-directory", GTK_STOCK_DIRECTORY,
      N_("New _Directory..."), NULL,
      N_("Create a directory"), G_CALLBACK(createDirectory) },
    { "configure", GTK_STOCK_PROPERTIES, N_("Confi_gure"), NULL,
      N_("Configure the selected object"), G_CALLBACK(configure) },
    { "project-manage-configurations", NULL, N_("Manage Confi_gurations"), NULL,
      N_("Manage configurations of the current project"),
      G_CALLBACK(manageConfigurations) },

    // File menu.
    { "file-open", GTK_STOCK_OPEN, N_("_Open"), NULL,
      N_("Open the selected file"), G_CALLBACK(openFile) },
    { "file-open-new-editor", NULL, N_("Open in New _Editor"), NULL,
      N_("Open the selected file in a new editor"),
      G_CALLBACK(openFileInNewEditor) },
    { "file-save", GTK_STOCK_SAVE, N_("_Save"), NULL,
      N_("Save the current file"), G_CALLBACK(saveFile) },
    { "file-save-all", NULL, N_("Save _All"), NULL,
      N_("Save all edited files"), G_CALLBACK(saveAllFiles) },
    { "file-reload", GTK_STOCK_REVERT_TO_SAVED, N_("_Reload"), NULL,
      N_("Reload the curren file"), G_CALLBACK(reloadFile) },
    { "file-close", GTK_STOCK_CLOSE, N_("_Close"), NULL,
      N_("Close the current file"), G_CALLBACK(closeFile) },
    { "file-close-all", NULL, N_("C_lose All"), NULL,
      N_("Close all opened files"), G_CALLBACK(closeAllFiles) },
    { "file-page-setup", GTK_STOCK_PAGE_SETUP, N_("Page _Setup..."), NULL,
      N_("Set up the page settings for printing"), G_CALLBACK(setupPage) },
    { "file-print-preview", GTK_STOCK_PRINT_REVIEW, N_("Print Pre_view"), NULL,
      N_("Preview the printed file"), G_CALLBACK(previewPrintedFile) },
    { "file-print", GTK_STOCK_PRINT, N_("_Print"), NULL,
      N_("Print the current file"), G_CALLBACK(printFile) },

    // Edit menu.
    { "edit-undo", GTK_STOCK_UNDO, N_("_Undo"), "<control>Z",
      N_("Undo the last operation"), G_CALLBACK(undo) },
    { "edit-redo", GTK_STOCK_REDO, N_("_Redo"), "<shift><control>Z",
      N_("Redo the last undone operation"), G_CALLBACK(redo) },
    { "edit-cut", GTK_STOCK_CUT, N_("Cu_t"), "<control>X",
      N_("Cut the selected object"), G_CALLBACK(cut) },
    { "edit-copy", GTK_STOCK_COPY, N_("_Copy"), "<control>C",
      N_("Copy the selected object"), G_CALLBACK(copy) },
    { "edit-paste", GTK_STOCK_PASTE, N_("_Paste"), "<control>V",
      N_("Paste the object in the clipboard"), G_CALLBACK(paste) },
    { "edit-delete", GTK_STOCK_DELETE, N_("_Delete"), NULL,
      N_("Delete the selected object"), G_CALLBACK(deleteObject) },
    { "edit-preferences", GTK_STOCK_PREFERENCES, N_("Pre_ferences"), NULL,
      N_("Edit your preferences"), G_CALLBACK(editPreferences) },

    // View menu.
    { "view-new-window", NULL, N_("New _Window"), NULL,
      N_("Create a window"), G_CALLBACK(createWindow) },
    { "view-new-editor-group", NULL, N_("New Editor _Group"), NULL,
      N_("Create an editor group"), G_CALLBACK(createEditorGroup) },
    { "view-full-screen", GTK_STOCK_FULLSCREEN, N_("_Full Screen"), "F11",
      N_("Enter full screen mode"), G_CALLBACK(enterFullScreen) },

    // Help menu.
    { "help-manual", GTK_STOCK_HELP, N_("_Manual"), "F1",
      N_("Show the user manual"), G_CALLBACK(showManual) },
    { "help-tutorial", NULL, N_("_Tutorial"), NULL,
      N_("Show the tutorial"), G_CALLBACK(showTutorial) },
    { "help-about", GTK_STOCK_ABOUT, N_("_About"), NULL,
      N_("About Samoyed"), G_CALLBACK(showAbout) },

    // New popup menu.
    { "new-file", GTK_STOCK_FILE, N_("_File"), NULL,
      N_("Create a file"), G_CALLBACK(createFile) },
    { "new-directory", GTK_STOCK_DIRECTORY, N_("_Directory"), NULL,
      N_("Create a directory"), G_CALLBACK(createDirectory) }
};

namespace Samoyed
{

Actions::Actions(Window *window)
{
    m_actionGroup = gtk_action_group_new("actions");
    gtk_action_group_set_translation_domain(m_actionGroup, NULL);
    gtk_action_group_add_actions(m_actionGroup,
                                 actionEntries,
                                 N_ACTIONS,
                                 window);

    // Fill the action array.
    for (int i = 0; i < N_ACTIONS; i++)
        m_actions[i] = gtk_action_group_get_action(m_actionGroup,
                                                   actionEntries[i].name);
}

Actions::~Actions()
{
    g_object_unref(m_actionGroup);
}

void Actions::updateSensitivity(const Window *window)
{
}

}
