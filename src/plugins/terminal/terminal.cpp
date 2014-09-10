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
#ifdef OS_WINDOWS
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

#ifdef OS_WINDOWS

void setFont(GtkWidget *view, const char *font)
{
    PangoFontDescription *fontDesc = pango_font_description_from_string(font);
    gtk_widget_override_font(view, fontDesc);
    pango_font_description_free(fontDesc);
}

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
#ifdef OS_WINDOWS

    GtkWidget *sw;

    HANDLE outputWrite = INVALID_HANDLE_VALUE;
    HANDLE inputRead = INVALID_HANDLE_VALUE;
    HANDLE errorWrite = INVALID_HANDLE_VALUE;

    SECURITY_ATTRIBUTES sa;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&m_outputRead, &outputWrite, &sa, 0))
        goto ERROR_OUT;
    if (!DuplicateHandle(GetCurrentProcess(), outputWrite,
                         GetCurrentProcess(), &errorWrite,
                         0, TRUE, DUPLICATE_SAME_ACCESS))
        goto ERROR_OUT;
    if (!CreatePipe(&inputRead, &m_inputWrite, &sa, 0))
        goto ERROR_OUT;
    if (!SetHandleInformation(m_outputRead, HANDLE_FLAG_INHERIT, 0))
        goto ERROR_OUT;
    if (!SetHandleInformation(m_inputWrite, HANDLE_FLAG_INHERIT, 0))
        goto ERROR_OUT;

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = outputWrite;
    si.hStdInput = inputRead;
    si.hStdError = errorWrite;

    if (!CreateProcess(TEXT("C:\\Windows\\System32\\cmd.exe"), NULL,
                       NULL, NULL, TRUE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed failed to setup a terminal emulator."));
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to start shell "
              "\"C:\\Windows\\System32\\cmd.exe\"."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        goto ERROR_OUT;
    }
    m_process = pi.hProcess;
    if (!CloseHandle(pi.hThread))
        goto ERROR_OUT;
    if (!CloseHandle(outputWrite))
    {
        outputWrite = INVALID_HANDLE_VALUE;
        goto ERROR_OUT;
    }
    if (!CloseHandle(inputRead))
    {
        inputRead = INVALID_HANDLE_VALUE;
        goto ERROR_OUT;
    }
    if (!CloseHandle(errorWrite))
    {
        errorWrite = INVALID_HANDLE_VALUE;
        goto ERROR_OUT;
    }

    m_terminal = gtk_text_view_new();
    setFont(m_terminal, "Monospace");
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), m_terminal);

    m_outputReader = new boost::thread();
// TESTING
{
    DWORD nBytesWritten;
    WriteFile(hInputWrite, "dir\r\n", strlen("dir\r\n"), &nBytesWritten, NULL);
    char buffer[256];
    DWORD nBytesRead;
    bool exit;
    for (int i = 0; i < 10; i++)
    {
        if (!ReadFile(hOutputRead, buffer, sizeof(buffer), &nBytesRead, NULL))
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
                break;
        }
        gtk_text_buffer_insert_at_cursor(
            gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_terminal)),
            buffer, nBytesRead);
        if (!exit)
        {
        WriteFile(hInputWrite, "exit\r\n", strlen("exit\r\n"), &nBytesWritten, NULL);
        exit = true;
        }
    }
}

    return sw;

ERROR_OUT:
    if (outputWrite != INVALID_HANDLE_VALUE)
        CloseHandle(outputWrite);
    if (inputRead != INVALID_HANDLE_VALUE)
        CloseHandle(inputRead);
    if (errorWrite != INVALID_HANDLE_VALUE)
        CloseHandle(errorWrite);
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
#ifdef OS_WINDOWS
    if (m_process != INVALID_HANDLE_VALUE)
        CloseHandle(m_process);
    if (m_outputRead != INVALID_HANDLE_VALUE)
        CloseHandle(m_outputRead);
    if (m_inputWrite != INVALID_HANDLE_VALUE)
        CloseHandle(m_inputWrite);
#endif
}

void Terminal::grabFocus()
{
    gtk_widget_grab_focus(m_terminal);
}

}

}
