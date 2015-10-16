// Terminal.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "terminal.hpp"
#include "terminal-view.hpp"
#include "application.hpp"
#include "window/window.hpp"
#include "utilities/miscellaneous.hpp"
#include <string.h>
#ifndef OS_WINDOWS
# include <pwd.h>
# include <unistd.h>
# include <sys/types.h>
#endif
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <vte/vte.h>

namespace
{

void closeTerminal(VteTerminal *realTerm,
		           int status,
		           Samoyed::Terminal::Terminal *term)
{
    term->view().close();
}

}

namespace Samoyed
{

namespace Terminal
{

GtkWidget *Terminal::setup()
{
    m_terminal = vte_terminal_new();

    char *argv[3] = { NULL, NULL, NULL };
    char **envv = NULL;
#ifdef OS_WINDOWS
    char *instDir =
        g_win32_get_package_installation_directory_of_module(NULL);
    argv[0] = g_strconcat(instDir, "\\bin\\bash.exe", NULL);
    if (!g_file_test(argv[0], G_FILE_TEST_IS_EXECUTABLE))
    {
        char *base = g_path_get_basename(instDir);
        if (strcmp(base, "mingw32") == 0 ||
            strcmp(base, "mingw64") == 0)
        {
            char *parent = g_path_get_dirname(instDir);
            g_free(argv[0]);
            argv[0] = g_strconcat(parent, "\\usr\\bin\\bash.exe", NULL);
            g_free(parent);
        }
        g_free(base);
    }
    g_free(instDir);
    argv[1] = g_strdup("-l");
    if (!g_getenv("MSYSTEM"))
    {
        envv = g_get_environ();
        envv = g_environ_setenv(envv, "MSYSTEM", "MINGW32", FALSE);
    }
#else
    const char *shell = g_getenv("SHELL");
    if (shell)
        argv[0] = g_strdup(shell);
    else
    {
        struct passwd *pw;
        pw = getpwuid(getuid());
        if (pw)
            argv[0] = g_strdup(pw->pw_shell);
        else if (g_file_test("/usr/bin/bash", G_FILE_TEST_IS_EXECUTABLE))
            argv[0] = g_strdup("/usr/bin/bash");
        else
            argv[0] = g_strdup("/bin/sh");
    }
#endif
    GError *error = NULL;
#if VTE_CHECK_VERSION(0, 38, 0)
    if (!vte_terminal_spawn_sync(VTE_TERMINAL(m_terminal),
                                 VTE_PTY_DEFAULT,
                                 NULL, argv, envv,
                                 G_SPAWN_CHILD_INHERITS_STDIN,
                                 NULL, NULL, NULL, NULL, &error))
#else
    if (!vte_terminal_fork_command_full(VTE_TERMINAL(m_terminal),
                                        VTE_PTY_DEFAULT,
                                        NULL, argv, envv,
                                        G_SPAWN_CHILD_INHERITS_STDIN,
                                        NULL, NULL, NULL, &error))
#endif
    {
        GtkWidget *dialog = gtk_message_dialog_new(
			Application::instance().currentWindow() ?
            GTK_WINDOW(Application::instance().currentWindow()->gtkWidget()) :
			NULL,
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to setup a terminal emulator."));
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to start shell \"%s\". %s."),
            argv[0], error->message);
        g_error_free(error);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        gtk_widget_destroy(m_terminal);
        g_free(argv[0]);
        g_free(argv[1]);
        g_strfreev(envv);
        return NULL;
    }
    g_free(argv[0]);
    g_free(argv[1]);
    g_strfreev(envv);
    g_signal_connect(m_terminal, "child-exited",
                     G_CALLBACK(closeTerminal), this);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(grid), m_terminal, NULL,
                            GTK_POS_RIGHT, 1, 1);
    GtkWidget *vertScrollBar = gtk_scrollbar_new(
        GTK_ORIENTATION_VERTICAL,
        gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(m_terminal)));
    gtk_grid_attach_next_to(GTK_GRID(grid), vertScrollBar, m_terminal,
                            GTK_POS_RIGHT, 1, 1);
#ifdef OS_WINDOWS
    vte_terminal_set_rewrap_on_resize(VTE_TERMINAL(m_terminal), FALSE);
    gtk_widget_set_hexpand(m_terminal, TRUE);
    gtk_widget_set_vexpand(m_terminal, TRUE);
#endif
    return grid;
}

void Terminal::destroy()
{
    delete this;
}

Terminal::Terminal(TerminalView &view):
    m_view(view)
{
}

Terminal::~Terminal()
{
}

void Terminal::grabFocus()
{
    gtk_widget_grab_focus(m_terminal);
}

}

}
