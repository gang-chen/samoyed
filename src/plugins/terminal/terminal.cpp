// Terminal.
// Copyright (C) 2014 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "terminal.hpp"
#include "terminal-view.hpp"
#include "application.hpp"
#include "ui/window.hpp"
#include "utilities/miscellaneous.hpp"
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#ifdef G_OS_WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <unistd.h>
# include <sys/types.h>
# include <pwd.h>
# include <vte/vte.h>
#endif

namespace
{

#ifdef G_OS_WIN32


#else

void closeTerminal(VteTerminal *realTerm, Samoyed::Terminal::Terminal *term)
{
    term->view().close();
}

#endif

}

namespace Samoyed
{

namespace Terminal
{

GtkWidget *Terminal::setup()
{
#ifdef G_OS_WIN32

    GtkWidget *sw;

    // Reference: http://support.microsoft.com/kb/190351.
    HANDLE hOutputReadTmp = INVALID_HANDLE_VALUE,
           hOutputRead = INVALID_HANDLE_VALUE,
           hOutputWrite = INVALID_HANDLE_VALUE;
    HANDLE hInputWriteTmp = INVALID_HANDLE_VALUE,
           hInputRead = INVALID_HANDLE_VALUE,
           hInputWrite = INVALID_HANDLE_VALUE;
    HANDLE hErrorWrite = INVALID_HANDLE_VALUE;

    SECURITY_ATTRIBUTES sa;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&hOutputReadTmp, &hOutputWrite, &sa, 0))
        goto ERROR_OUT;
    if (!DuplicateHandle(GetCurrentProcess(), hOutputWrite,
                         GetCurrentProcess(), &hErrorWrite,
                         0, TRUE, DUPLICATE_SAME_ACCESS))
        goto ERROR_OUT;
    if (!CreatePipe(&hInputRead, &hInputWriteTmp, &sa, 0))
        goto ERROR_OUT;
    if (!DuplicateHandle(GetCurrentProcess(), hOutputReadTmp,
                         GetCurrentProcess(), &hOutputRead,
                         0, FALSE, DUPLICATE_SAME_ACCESS))
        goto ERROR_OUT;
    if (!DuplicateHandle(GetCurrentProcess(), hInputWriteTmp,
                         GetCurrentProcess(), &hInputWrite,
                         0, FALSE, DUPLICATE_SAME_ACCESS))
        goto ERROR_OUT;
    if (!CloseHandle(hOutputReadTmp))
    {
        hOutputReadTmp = INVALID_HANDLE_VALUE;
        goto ERROR_OUT;
    }
    hOutputReadTmp = INVALID_HANDLE_VALUE;
    if (!CloseHandle(hInputWriteTmp))
    {
        hInputWriteTmp = INVALID_HANDLE_VALUE;
        goto ERROR_OUT;
    }
    hInputWriteTmp = INVALID_HANDLE_VALUE;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hOutputWrite;
    si.hStdInput = hInputRead;
    si.hStdError = hErrorWrite;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcess(TEXT("C:\\Windows\\System32\\cmd.exe"), TEXT(""),
                       NULL, NULL, TRUE,
                       CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi))
        goto ERROR_OUT;
    m_process = pi.hProcess;
    if (!CloseHandle(pi.hThread))
        goto ERROR_OUT;

    m_terminal = gtk_text_view_new();
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), m_terminal);

// TESTING
{
    DWORD nBytesWritten;
    WriteFile(hInputWrite, "dir\r\n", strlen("dir\r\n"), &nBytesWritten, NULL);
    char buffer[256];
    DWORD nBytesRead;
    for (int i = 0; i < 10; i++)
    {
        if (!ReadFile(hOutputRead, buffer, sizeof(buffer), &nBytesRead, NULL))
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
                break;
            break;
        }
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_terminal)),
            buffer, nBytesRead);
        WriteFile(hInputWrite, "exit\r\n", strlen("exit\r\n"), &nBytesWritten, NULL);
    }
}

    return sw;

ERROR_OUT:
    if (hOutputReadTmp != INVALID_HANDLE_VALUE)
        CloseHandle(hOutputReadTmp);
    return NULL;

#else

    m_terminal = vte_terminal_new();

    struct passwd *pw;
    const char *shell;
    const char *dir;
    pw = getpwuid(getuid());
    if (pw)
    {
        shell = pw->pw_shell;
        dir = pw->pw_dir;
    } else {
        shell = "/bin/sh";
        dir = "/";
    }

    char *argv[2];
    argv[0] = strdup(shell);
    argv[1] = NULL;
    GError *error = NULL;
    if (!vte_terminal_fork_command_full(VTE_TERMINAL(m_terminal),
                                        VTE_PTY_DEFAULT,
                                        dir, argv, NULL,
                                        G_SPAWN_CHILD_INHERITS_STDIN,
                                        NULL, NULL, NULL, &error))
    {
        free(argv[0]);
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to setup a terminal emulator."));
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to start shell \"%s\". %s."),
            shell, error->message);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_error_free(error);
        gtk_widget_destroy(m_terminal);
        return NULL;
    }
    free(argv[0]);
    g_signal_connect(m_terminal, "child-exited",
                     G_CALLBACK(closeTerminal), this);

    GtkWidget *sb = gtk_scrollbar_new(
        GTK_ORIENTATION_VERTICAL,
        gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(m_terminal)));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(grid), m_terminal, NULL,
                            GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(grid), sb, m_terminal,
                            GTK_POS_RIGHT, 1, 1);
    return grid;

#endif
}

Terminal::~Terminal()
{
    if (m_process != INVALID_HANDLE_VALUE)
        CloseHandle(m_process);
}

void Terminal::grabFocus()
{
    gtk_widget_grab_focus(m_terminal);
}

}

}
