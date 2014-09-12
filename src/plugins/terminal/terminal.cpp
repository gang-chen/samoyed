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
#include <stdio.h>
#include <string.h>
#include <utility>
#include <string>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#ifdef OS_WINDOWS
# include <boost/thread.hpp>
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
FILE *fff;
const char *DEFAULT_FONT = "Monospace 10";
const char *DEFAULT_COLOR = "white";
const char *DEFAULT_BACKGROUND_COLOR = "black";
const int DEFAULT_OUTPUT_READ_INTERVAL = 100000000;

class OutputReader
{
public:
    OutputReader(Samoyed::Terminal::Terminal &term): m_terminal(term) {}

    void operator()();

private:
    Samoyed::Terminal::Terminal &m_terminal;
};

void OutputReader::operator()()
{
    char *error = NULL;
    char buffer[BUFSIZ];
    DWORD length;
    for (;;)
    {
        if (!ReadFile(m_terminal.outputReadHandle(),
                      buffer, sizeof(buffer), &length, NULL))
        {
            if (GetLastError() == ERROR_BROKEN_PIPE)
            {fprintf(fff,"read output done\n");fflush(fff);
                break;
                }
            error = g_win32_error_message(GetLastError());
            fprintf(fff,"read output error %s\n", error);fflush(fff);
            break;
        }
        if (length)
        {fprintf(fff,"read some output %s\n", buffer);fflush(fff);
            m_terminal.onOutput(buffer, length);
            }
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.nsec += DEFAULT_OUTPUT_READ_INTERVAL;
        boost::thread::sleep(xt);
    }
    m_terminal.onOutputDone(error);
}

class InputWriter
{
public:
    InputWriter(Samoyed::Terminal::Terminal &term, char *content, int length):
        m_terminal(term),
        m_length(length)
    {
        m_content = new char[length];
        memcpy(m_content, content, sizeof(char) * length);
    }

    ~InputWriter()
    {
        delete[] m_content;
    }

    void operator()();

private:
    Samoyed::Terminal::Terminal &m_terminal;
    char *m_content;
    int m_length;
};

void InputWriter::operator()()
{
    char *error = NULL;
    DWORD length;
        fprintf(fff,"to write input %s (%d)\n", m_content, m_length);fflush(fff);
    for (int lengthWritten = 0;
         lengthWritten < m_length;
         lengthWritten += length)
    {
        if (!WriteFile(m_terminal.inputWriteHandle(),
                       m_content + lengthWritten, m_length - lengthWritten,
                       &length, NULL))
        {
            error = g_win32_error_message(GetLastError());
            fprintf(fff,"write input error %s\n", error);fflush(fff);
            break;
        }
        fprintf(fff,"write input %s (%d)\n", m_content + lengthWritten, length);fflush(fff);
    }
    m_terminal.onInputDone(error);
}

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

#ifdef OS_WINDOWS

gboolean Terminal::onOutputInMainThread(gpointer param)
{
    std::pair<Terminal *, std::string> *p =
        static_cast<std::pair<Terminal *, std::string> *>(param);
    p->first->m_outputting = true;
    gtk_text_buffer_insert_at_cursor(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(p->first->m_terminal)),
        p->second.c_str(), p->second.length());
    p->first->m_outputting = false;
    delete p;
    return FALSE;
}

void Terminal::onOutput(char *content, int length)
{
    g_idle_add(onOutputInMainThread,
               new std::pair<Terminal *, std::string>
                (this, std::string(content, length)));
}

gboolean Terminal::onOutputDoneInMainThread(gpointer param)
{
    std::pair<Terminal *, char *> *p =
        static_cast<std::pair<Terminal *, char *> *>(param);
    delete p->first->m_outputReader;
    p->first->m_outputReader = NULL;
    if (p->second)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed terminal emulator encountered a fatal error. It will be "
              "shut down."));
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to receive output from the shell. %s"),
            p->second);
        g_free(p->second);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    p->first->view().close();
    delete p;
    return FALSE;
}

void Terminal::onOutputDone(char *error)
{
    g_idle_add(onOutputDoneInMainThread,
               new std::pair<Terminal *, char *>(this, error));
}

gboolean Terminal::onInputDoneInMainThread(gpointer param)
{
    std::pair<Terminal *, char *> *p =
        static_cast<std::pair<Terminal *, char *> *>(param);
    delete p->first->m_inputWriter;
    p->first->m_inputWriter = NULL;
    if (p->second)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
            GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            _("Samoyed terminal emulator encountered a fatal error. It will be "
              "shut down."));
        gtkMessageDialogAddDetails(
            dialog,
            _("Samoyed failed to send commands to the shell. %s"),
            p->second);
        g_free(p->second);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        p->first->view().close();
    }
    delete p;
    return FALSE;
}

void Terminal::onInputDone(char *error)
{
    g_idle_add(onInputDoneInMainThread,
               new std::pair<Terminal *, char *>(this, error));
}

void Terminal::onInsertText(GtkTextBuffer *buffer, GtkTextIter *location,
                            char *text, int length,
                            Terminal *term)
{
    if (term->m_outputting)
        return;
    bool entered = false;
    for (int i = 0; i < length; i++)
        if (text[i] == '\n')
        {
            entered = true;
            break;
        }
    if (!entered)
        return;
    //term->m_inputWriter = new boost::thread(InputWriter(*term, text, length));
InputWriter x(*term,text,length);
x();
}

#endif

GtkWidget *Terminal::setup()
{
#ifdef OS_WINDOWS
fff=fopen("testmsg.txt","w");
    GtkWidget *sw;
    std::string error;

    HANDLE outputWriteHandle = INVALID_HANDLE_VALUE;
    HANDLE inputReadHandle = INVALID_HANDLE_VALUE;
    HANDLE errorWriteHandle = INVALID_HANDLE_VALUE;

    SECURITY_ATTRIBUTES sa;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&sa, sizeof(SECURITY_ATTRIBUTES));
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if (!CreatePipe(&m_outputReadHandle, &outputWriteHandle, &sa, 0))
    {
        error = _("Samoyed failed to create a pipe.");
        goto ERROR_OUT;
    }
    if (!DuplicateHandle(GetCurrentProcess(), outputWriteHandle,
                         GetCurrentProcess(), &errorWriteHandle,
                         0, TRUE, DUPLICATE_SAME_ACCESS))
    {
        error = _("Samoyed failed to duplicate a handle.");
        goto ERROR_OUT;
    }
    if (!CreatePipe(&inputReadHandle, &m_inputWriteHandle, &sa, 0))
    {
        error = _("Samoyed failed to create a pipe.");
        goto ERROR_OUT;
    }
    if (!SetHandleInformation(m_outputReadHandle, HANDLE_FLAG_INHERIT, 0))
    {
        error = _("Samoyed failed to make a handle noninheritable.");
        goto ERROR_OUT;
    }
    if (!SetHandleInformation(m_inputWriteHandle, HANDLE_FLAG_INHERIT, 0))
    {
        error = _("Samoyed failed to make a handle noninheritable.");
        goto ERROR_OUT;
    }

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = outputWriteHandle;
    si.hStdInput = inputReadHandle;
    si.hStdError = errorWriteHandle;

    if (!CreateProcess(TEXT("C:\\Windows\\System32\\cmd.exe"), NULL,
                       NULL, NULL, TRUE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        error = _("Samoyed failed to start shell "
                  "\"C:\\Windows\\System32\\cmd.exe\".");
        goto ERROR_OUT;
    }
    m_process = pi.hProcess;
    if (!CloseHandle(pi.hThread))
    {
        error = _("Samoyed failed to close a handle.");
        goto ERROR_OUT;
    }
    if (!CloseHandle(outputWriteHandle))
    {
        error = _("Samoyed failed to close a handle.");
        outputWriteHandle = INVALID_HANDLE_VALUE;
        goto ERROR_OUT;
    }
    if (!CloseHandle(inputReadHandle))
    {
        error = _("Samoyed failed to close a handle.");
        inputReadHandle = INVALID_HANDLE_VALUE;
        goto ERROR_OUT;
    }
    if (!CloseHandle(errorWriteHandle))
    {
        error = _("Samoyed failed to close a handle.");
        errorWriteHandle = INVALID_HANDLE_VALUE;
        goto ERROR_OUT;
    }

    m_terminal = gtk_text_view_new();
    setFont(m_terminal, DEFAULT_FONT);
    GdkRGBA color;
    gdk_rgba_parse(&color, DEFAULT_COLOR);
    gtk_widget_override_color(m_terminal, GTK_STATE_FLAG_NORMAL, &color);
    gdk_rgba_parse(&color, DEFAULT_BACKGROUND_COLOR);
    gtk_widget_override_background_color(m_terminal,
                                         GTK_STATE_FLAG_NORMAL,
                                         &color);
    g_signal_connect(gtk_text_view_get_buffer(GTK_TEXT_VIEW(m_terminal)),
                     "insert-text",
                     G_CALLBACK(onInsertText), this);
    sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(sw), m_terminal);

    m_outputReader = new boost::thread(OutputReader(*this));

    return sw;

ERROR_OUT:
    char *sysMsg = g_win32_error_message(GetLastError());
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(Application::instance().currentWindow().gtkWidget()),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        _("Samoyed failed to setup a terminal emulator."));
    gtkMessageDialogAddDetails(dialog, "%s %s", error.c_str(), sysMsg);
    g_free(sysMsg);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (outputWriteHandle != INVALID_HANDLE_VALUE)
        CloseHandle(outputWriteHandle);
    if (inputReadHandle != INVALID_HANDLE_VALUE)
        CloseHandle(inputReadHandle);
    if (errorWriteHandle != INVALID_HANDLE_VALUE)
        CloseHandle(errorWriteHandle);
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
    if (m_outputReadHandle != INVALID_HANDLE_VALUE)
        CloseHandle(m_outputReadHandle);
    if (m_inputWriteHandle != INVALID_HANDLE_VALUE)
        CloseHandle(m_inputWriteHandle);
#endif
}

void Terminal::grabFocus()
{
    gtk_widget_grab_focus(m_terminal);
}

}

}
