// Actions.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "actions.hpp"
#include "window.hpp"
#include "file.hpp"
#include "editor.hpp"
#include "notebook.hpp"
#include "application.hpp"
#include <gtk/gtk.h>
#include <glib/gi18n.h>

namespace
{

void createSession(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Application::instance().createSession();
}

void switchSession(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Application::instance().switchSession();
}

void manageSessions(GtkAction *action, Samoyed::Window *window)
{
}

void quitSession(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Application::instance().quit();
}

void createProject(GtkAction *action, Samoyed::Window *window)
{
}

void openProject(GtkAction *action, Samoyed::Window *window)
{
}

void closeProject(GtkAction *action, Samoyed::Window *window)
{
}

void closeAllProjects(GtkAction *action, Samoyed::Window *window)
{
}

void createFile(GtkAction *action, Samoyed::Window *window)
{
}

void createDirectory(GtkAction *action, Samoyed::Window *window)
{
}

void configure(GtkAction *action, Samoyed::Window *window)
{
}

void manageConfigurations(GtkAction *action, Samoyed::Window *window)
{
}

void openFile(GtkAction *action, Samoyed::Window *window)
{
    std::list<std::pair<Samoyed::File *, Samoyed::Editor *> > opened;
    Samoyed::File::openByDialog(NULL, opened);
    if (!opened.empty())
        opened.back().second->setCurrent();
}

void saveFile(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Notebook &editorGroup = window->currentEditorGroup();
    if (editorGroup.childCount() > 0)
    {
        Samoyed::File &file =
            static_cast<Samoyed::Editor &>(editorGroup.currentChild()).file();
        if (file.edited() && !file.frozen() && !file.inEditGroup())
            file.save();
    }
}

void saveAllFiles(GtkAction *action, Samoyed::Window *window)
{
    for (Samoyed::File *file = Samoyed::Application::instance().files();
         file;
         file = file->next())
    {
        if (file->edited() && !file->frozen() && !file->inEditGroup())
            file->save();
    }
}

void reloadFile(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Notebook &editorGroup = window->currentEditorGroup();
    if (editorGroup.childCount() > 0)
    {
        Samoyed::File &file =
            static_cast<Samoyed::Editor &>(editorGroup.currentChild()).file();
        if (!file.frozen() && !file.inEditGroup())
            file.load(true);
    }
}

void closeFile(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Notebook &editorGroup = window->currentEditorGroup();
    if (editorGroup.childCount() > 0)
        static_cast<Samoyed::Editor &>(editorGroup.currentChild()).file().
            close();
}

void closeAllFiles(GtkAction *action, Samoyed::Window *window)
{
    for (Samoyed::File *file = Samoyed::Application::instance().files();
         file;
         file = file->next())
        file->close();
}

void undo(GtkAction *action, Samoyed::Window *window)
{
    window->current().activateAction(*window, action);
}

void redo(GtkAction *action, Samoyed::Window *window)
{
    window->current().activateAction(*window, action);
}

void cut(GtkAction *action, Samoyed::Window *window)
{
    window->current().activateAction(*window, action);
}

void copy(GtkAction *action, Samoyed::Window *window)
{
    window->current().activateAction(*window, action);
}

void paste(GtkAction *action, Samoyed::Window *window)
{
    window->current().activateAction(*window, action);
}

void deleteObject(GtkAction *action, Samoyed::Window *window)
{
    window->current().activateAction(*window, action);
}

void editPreferences(GtkAction *action, Samoyed::Window *window)
{
}

void createWindow(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Window::Configuration config = window->configuration();
    Samoyed::Window::create(config);
}

void createEditor(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Notebook &editorGroup = window->currentEditorGroup();
    if (editorGroup.childCount() > 0)
    {
        Samoyed::Editor *editor =
            static_cast<Samoyed::Editor &>(editorGroup.currentChild()).file().
            createEditor(NULL);
        window->addEditorToEditorGroup(*editor, editorGroup,
                                       editorGroup.currentChildIndex());
        editor->setCurrent();
    }
}

void moveEditorDown(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Notebook &editorGroup = window->currentEditorGroup();
    if (editorGroup.childCount() > 1)
    {
        Samoyed::Editor &editor =
            static_cast<Samoyed::Editor &>(editorGroup.currentChild());
        Samoyed::Notebook *newEditorGroup =
            window->neighborEditorGroup(editorGroup,
                                        Samoyed::Window::SIDE_BOTTOM);
        if (!newEditorGroup)
            newEditorGroup =
                window->splitEditorGroup(editorGroup,
                                         Samoyed::Window::SIDE_BOTTOM);
        editorGroup.removeChild(editor);
        window->addEditorToEditorGroup(
            editor,
            *newEditorGroup,
            newEditorGroup->currentChildIndex() + 1);
        editor.setCurrent();
    }
}

void moveEditorRight(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Notebook &editorGroup = window->currentEditorGroup();
    if (editorGroup.childCount() > 1)
    {
        Samoyed::Editor &editor =
            static_cast<Samoyed::Editor &>(editorGroup.currentChild());
        Samoyed::Notebook *newEditorGroup =
            window->neighborEditorGroup(editorGroup,
                                        Samoyed::Window::SIDE_RIGHT);
        if (!newEditorGroup)
            newEditorGroup =
                window->splitEditorGroup(editorGroup,
                                         Samoyed::Window::SIDE_RIGHT);
        editorGroup.removeChild(editor);
        window->addEditorToEditorGroup(
            editor,
            *newEditorGroup,
            newEditorGroup->currentChildIndex() + 1);
        editor.setCurrent();
    }
}

void splitEditorVertically(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Notebook &editorGroup = window->currentEditorGroup();
    if (editorGroup.childCount() > 0)
    {
        Samoyed::Editor *editor =
            static_cast<Samoyed::Editor &>(editorGroup.currentChild()).file().
            createEditor(NULL);
        Samoyed::Notebook *newEditorGroup =
            window->splitEditorGroup(editorGroup,
                                     Samoyed::Window::SIDE_BOTTOM);
        window->addEditorToEditorGroup(*editor, *newEditorGroup, 0);
        editor->setCurrent();
    }
}

void splitEditorHorizontally(GtkAction *action, Samoyed::Window *window)
{
    Samoyed::Notebook &editorGroup = window->currentEditorGroup();
    if (editorGroup.childCount() > 0)
    {
        Samoyed::Editor *editor =
            static_cast<Samoyed::Editor &>(editorGroup.currentChild()).file().
            createEditor(NULL);
        Samoyed::Notebook *newEditorGroup =
            window->splitEditorGroup(editorGroup,
                                     Samoyed::Window::SIDE_RIGHT);
        window->addEditorToEditorGroup(*editor, *newEditorGroup, 0);
        editor->setCurrent();
    }
}

void showManual(GtkAction *action, Samoyed::Window *window)
{
}

void showTutorial(GtkAction *action, Samoyed::Window *window)
{
}

void showAbout(GtkAction *action, Samoyed::Window *window)
{
}

void showHideToolbar(GtkToggleAction *action, Samoyed::Window *window)
{
    window->setToolbarVisible(gtk_toggle_action_get_active(action));
}

void showHideStatusBar(GtkToggleAction *action, Samoyed::Window *window)
{
    window->setStatusBarVisible(gtk_toggle_action_get_active(action));
}

void enterLeaveFullScreen(GtkToggleAction *action, Samoyed::Window *window)
{
    if (gtk_toggle_action_get_active(action))
        window->enterFullScreen();
    else
        window->leaveFullScreen();
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
    { "create-session", NULL, N_("_New..."), NULL,
      N_("Quit the current session and start a new one"),
      G_CALLBACK(createSession) },
    { "switch-session", NULL, N_("_Switch..."), NULL,
      N_("Quit the current session and restore a saved one"),
      G_CALLBACK(switchSession) },
    { "manage-sessions", NULL, N_("_Manage"), NULL,
      N_("Manage saved sessions"), G_CALLBACK(manageSessions) },
    { "quit-session", "application-exit", N_("_Quit"), "<Control>q",
      N_("Quit the current session"), G_CALLBACK(quitSession) },

    // Project menu.
    { "create-project", NULL, N_("_New..."), NULL,
      N_("Create a project"), G_CALLBACK(createProject) },
    { "open-project", NULL, N_("_Open..."), NULL,
      N_("Open a project"), G_CALLBACK(openProject) },
    { "close-project", NULL, N_("_Close"), NULL,
      N_("Close the current project"), G_CALLBACK(closeProject) },
    { "close-all-projects", NULL, N_("Close _All"), NULL,
      N_("Close all opened projects"), G_CALLBACK(closeAllProjects) },
    { "create-file", "document-new", N_("New _File..."), NULL,
      N_("Create a file"), G_CALLBACK(createFile) },
    { "create-directory", NULL, N_("New _Directory..."), NULL,
      N_("Create a directory"), G_CALLBACK(createDirectory) },
    { "configure", "document-properties", N_("Confi_gure"), "<Alt>Return",
      N_("Configure the selected object"), G_CALLBACK(configure) },
    { "manage-configurations", NULL, N_("Manage Confi_gurations"), NULL,
      N_("Manage configurations of the current project"),
      G_CALLBACK(manageConfigurations) },

    // File menu.
    { "open-file", "document-open", N_("_Open..."), "<Control>o",
      N_("Open a file"), G_CALLBACK(openFile) },
    { "save-file", "document-save", N_("_Save"), "<Control>s",
      N_("Save the current file"), G_CALLBACK(saveFile) },
    { "save-all-files", NULL, N_("Save _All"), "<Shift><Control>s",
      N_("Save all edited files"), G_CALLBACK(saveAllFiles) },
    { "reload-file", "document-revert", N_("_Reload"), NULL,
      N_("Reload the curren file"), G_CALLBACK(reloadFile) },
    { "close-file", "window-close", N_("_Close"), "<Control>w",
      N_("Close the current file"), G_CALLBACK(closeFile) },
    { "close-all-files", NULL, N_("C_lose All"), "<Shift><Control>w",
      N_("Close all opened files"), G_CALLBACK(closeAllFiles) },

    // Edit menu.
    { "undo", "edit-undo", N_("_Undo"), "<Control>z",
      N_("Undo the last operation"), G_CALLBACK(undo) },
    { "redo", "edit-redo", N_("_Redo"), "<Shift><Control>z",
      N_("Redo the last undone operation"), G_CALLBACK(redo) },
    { "cut", "edit-cut", N_("Cu_t"), "<Control>x",
      N_("Cut the selected object"), G_CALLBACK(cut) },
    { "copy", "edit-copy", N_("_Copy"), "<Control>c",
      N_("Copy the selected object"), G_CALLBACK(copy) },
    { "paste", "edit-paste", N_("_Paste"), "<Control>v",
      N_("Paste the object in the clipboard"), G_CALLBACK(paste) },
    { "delete", "edit-delete", N_("_Delete"), NULL,
      N_("Delete the selected object"), G_CALLBACK(deleteObject) },
    { "edit-preferences", GTK_STOCK_PREFERENCES, N_("Pre_ferences"), NULL,
      N_("Edit your preferences"), G_CALLBACK(editPreferences) },

    // View menu.
    { "create-window", "window-new", N_("New _Window"), NULL,
      N_("Create a window"), G_CALLBACK(createWindow) },
    { "create-editor", NULL, N_("New _Editor"), NULL,
      N_("Create an editor for the current file"), G_CALLBACK(createEditor) },
    { "move-editor-down", NULL, N_("Move Editor _Down"), NULL,
      N_("Move the current editor to the editor group, or a new editor group "
         "if none, below the current one"),
      G_CALLBACK(moveEditorDown) },
    { "move-editor-right", NULL, N_("Move Editor _Right"), NULL,
      N_("Move the current editor to a new editor group, or a new editor group "
         "if none, to the right of the current one"),
      G_CALLBACK(moveEditorRight) },
    { "split-editor-vertically", NULL, N_("Split Editor _Vertically"), NULL,
      N_("Split the current editor vertically"),
      G_CALLBACK(splitEditorVertically) },
    { "split-editor-horizontally", NULL, N_("Split Editor _Horizontally"), NULL,
      N_("Split the current editor horizontally"),
      G_CALLBACK(splitEditorHorizontally) },

    // Help menu.
    { "show-manual", "help-contents", N_("_Manual"), "F1",
      N_("Show the user manual"), G_CALLBACK(showManual) },
    { "show-tutorial", NULL, N_("_Tutorial"), NULL,
      N_("Show the tutorial"), G_CALLBACK(showTutorial) },
    { "show-about", "help-about", N_("_About"), NULL,
      N_("About Samoyed"), G_CALLBACK(showAbout) }
};

const GtkToggleActionEntry
toggleActionEntries[Samoyed::Actions::N_TOGGLE_ACTIONS] =
{
    { "show-hide-toolbar", NULL, N_("_Toolbar"), NULL,
      N_("Show or hide toolbar"), G_CALLBACK(showHideToolbar), TRUE },
    { "show-hide-status-bar", NULL, N_("_Status Bar"), NULL,
      N_("Show or hide status bar"), G_CALLBACK(showHideStatusBar), TRUE },
    { "enter-leave-full-screen", "view-fullscreen",
      N_("_Full Screen"), "F11",
      N_("Enter or leave full screen mode"),
      G_CALLBACK(enterLeaveFullScreen), FALSE }
};

}

namespace Samoyed
{

bool Actions::s_sensitivityUpdaterAdded = false;

void Actions::invalidateSensitivity()
{
    if (s_sensitivityUpdaterAdded)
        return;
    s_sensitivityUpdaterAdded = true;
    g_idle_add_full(G_PRIORITY_HIGH_IDLE, updateSensitivity, NULL, NULL);
}

gboolean Actions::updateSensitivity(gpointer)
{
    for (Window *window = Application::instance().windows();
         window;
         window = window->next())
    {
        Actions &actions = window->actions();
        Notebook &editorGroup = window->currentEditorGroup();
        if (editorGroup.childCount() == 0 ||
            &editorGroup.current() != &window->current())
        {
            // No editor exists or the current widget is not an editor.
            gtk_action_set_sensitive(actions.action(ACTION_SAVE_FILE), FALSE);
            gtk_action_set_sensitive(actions.action(ACTION_RELOAD_FILE), FALSE);
            gtk_action_set_sensitive(actions.action(ACTION_CLOSE_FILE), FALSE);

            gtk_action_set_sensitive(actions.action(ACTION_CREATE_EDITOR),
                                     FALSE);
            gtk_action_set_sensitive(actions.action(ACTION_MOVE_EDITOR_DOWN),
                                     FALSE);
            gtk_action_set_sensitive(actions.action(ACTION_MOVE_EDITOR_RIGHT),
                                     FALSE);
            gtk_action_set_sensitive(
                actions.action(ACTION_SPLIT_EDITOR_VERTICALLY),
                FALSE);
            gtk_action_set_sensitive(
                actions.action(ACTION_SPLIT_EDITOR_HORIZONTALLY),
                FALSE);
        }
        else
        {
            File &file = static_cast<Editor &>(editorGroup.current()).file();
            if (file.edited() && !file.frozen() && !file.inEditGroup())
                gtk_action_set_sensitive(actions.action(ACTION_SAVE_FILE),
                                         TRUE);
            else
                gtk_action_set_sensitive(actions.action(ACTION_SAVE_FILE),
                                         FALSE);
            if (!file.frozen() && !file.inEditGroup())
                gtk_action_set_sensitive(actions.action(ACTION_RELOAD_FILE),
                                         TRUE);
            else
                gtk_action_set_sensitive(actions.action(ACTION_RELOAD_FILE),
                                         FALSE);
            gtk_action_set_sensitive(actions.action(ACTION_CLOSE_FILE), TRUE);

            gtk_action_set_sensitive(actions.action(ACTION_CREATE_EDITOR),
                                     TRUE);
            if (editorGroup.childCount() > 1)
            {
                gtk_action_set_sensitive(
                    actions.action(ACTION_MOVE_EDITOR_DOWN),
                    TRUE);
                gtk_action_set_sensitive(
                    actions.action(ACTION_MOVE_EDITOR_RIGHT),
                    TRUE);
            }
            else
            {
                gtk_action_set_sensitive(
                    actions.action(ACTION_MOVE_EDITOR_DOWN),
                    FALSE);
                gtk_action_set_sensitive(
                    actions.action(ACTION_MOVE_EDITOR_RIGHT),
                    FALSE);
            }
            gtk_action_set_sensitive(
                actions.action(ACTION_SPLIT_EDITOR_VERTICALLY),
                TRUE);
            gtk_action_set_sensitive(
                actions.action(ACTION_SPLIT_EDITOR_HORIZONTALLY),
                TRUE);
        }

        Widget &current = window->current();
        if (current.isActionSensitive(*window, actions.action(ACTION_UNDO)))
            gtk_action_set_sensitive(actions.action(ACTION_UNDO), TRUE);
        else
            gtk_action_set_sensitive(actions.action(ACTION_UNDO), FALSE);
        if (current.isActionSensitive(*window, actions.action(ACTION_REDO)))
            gtk_action_set_sensitive(actions.action(ACTION_REDO), TRUE);
        else
            gtk_action_set_sensitive(actions.action(ACTION_REDO), FALSE);
        if (current.isActionSensitive(*window, actions.action(ACTION_CUT)))
            gtk_action_set_sensitive(actions.action(ACTION_CUT), TRUE);
        else
            gtk_action_set_sensitive(actions.action(ACTION_CUT), FALSE);
        if (current.isActionSensitive(*window, actions.action(ACTION_COPY)))
            gtk_action_set_sensitive(actions.action(ACTION_COPY), TRUE);
        else
            gtk_action_set_sensitive(actions.action(ACTION_COPY), FALSE);
        if (current.isActionSensitive(*window, actions.action(ACTION_PASTE)))
            gtk_action_set_sensitive(actions.action(ACTION_PASTE), TRUE);
        else
            gtk_action_set_sensitive(actions.action(ACTION_PASTE), FALSE);
        if (current.isActionSensitive(*window, actions.action(ACTION_DELETE)))
            gtk_action_set_sensitive(actions.action(ACTION_DELETE), TRUE);
        else
            gtk_action_set_sensitive(actions.action(ACTION_DELETE), FALSE);

        window->updateActionsSensitivity();
    }

    s_sensitivityUpdaterAdded = false;
    return FALSE;
}

Actions::Actions(Window *window):
    m_window(window)
{
    m_actionGroup = gtk_action_group_new("actions");
    gtk_action_group_set_translation_domain(m_actionGroup, NULL);
    gtk_action_group_add_actions(m_actionGroup,
                                 actionEntries,
                                 N_ACTIONS,
                                 window);
    gtk_action_group_add_toggle_actions(m_actionGroup,
                                        toggleActionEntries,
                                        N_TOGGLE_ACTIONS,
                                        window);

    // Fill the action array.
    for (int i = 0; i < N_ACTIONS; ++i)
        m_actions[i] = gtk_action_group_get_action(m_actionGroup,
                                                   actionEntries[i].name);
    for (int i = 0; i < N_TOGGLE_ACTIONS; ++i)
        m_toggleActions[i] = GTK_TOGGLE_ACTION(
            gtk_action_group_get_action(m_actionGroup,
                                        toggleActionEntries[i].name));
}

Actions::~Actions()
{
    g_object_unref(m_actionGroup);
}

void Actions::onToolbarVisibilityChanged(bool visibility)
{
    gtk_toggle_action_set_active(toggleAction(TOGGLE_ACTION_SHOW_HIDE_TOOLBAR),
                                 visibility);
}

void Actions::onStatusBarVisibilityChanged(bool visibility)
{
    gtk_toggle_action_set_active(toggleAction(
                                     TOGGLE_ACTION_SHOW_HIDE_STATUS_BAR),
                                 visibility);
}

void Actions::onWindowFullScreenChanged(bool inFullScreen)
{
    gtk_toggle_action_set_active(toggleAction(
                                     TOGGLE_ACTION_ENTER_LEAVE_FULL_SCREEN),
                                 inFullScreen);
}

}
