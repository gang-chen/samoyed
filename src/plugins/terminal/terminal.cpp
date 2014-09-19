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
#include <stdlib.h>
#include <string.h>
#include <utility>
#include <string>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#ifdef OS_WINDOWS
# include <boost/noncopyable.hpp>
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
                break;
            error = g_win32_error_message(GetLastError());
            break;
        }
        if (length)
            m_terminal.onOutput(buffer, length);
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
    InputWriter(Samoyed::Terminal::Terminal &term,
                const char *content,
                int length):
        m_terminal(term),
        m_length(length)
    {
        m_content = static_cast<char *>(malloc(sizeof(char) * m_length));
        memcpy(m_content, content, sizeof(char) * m_length);
    }

    InputWriter(const InputWriter &i):
        m_terminal(i.m_terminal),
        m_length(i.m_length)
    {
        m_content = static_cast<char *>(malloc(sizeof(char) * m_length));
        memcpy(m_content, i.m_content, sizeof(char) * m_length);
    }

    ~InputWriter()
    {
        free(m_content);
    }

    void operator()();

private:
    const InputWriter &operator=(const InputWriter &rhs);

    Samoyed::Terminal::Terminal &m_terminal;
    char *m_content;
    int m_length;
};

void InputWriter::operator()()
{
    char *error = NULL;
    DWORD length;
    for (int lengthWritten = 0;
         lengthWritten < m_length;
         lengthWritten += length)
    {
        if (!WriteFile(m_terminal.inputWriteHandle(),
                       m_content + lengthWritten, m_length - lengthWritten,
                       &length, NULL))
        {
            error = g_win32_error_message(GetLastError());
            break;
        }
    }
    m_terminal.onInputDone(error);
}

class OutputParam: public boost::noncopyable
{
public:
    OutputParam(Samoyed::Terminal::Terminal &term,
                const char *content,
                int length):
        m_terminal(term),
        m_length(length)
    {
        m_content = static_cast<char *>(malloc(sizeof(char) * m_length));
        memcpy(m_content, content, sizeof(char) * m_length);
    }
    ~OutputParam()
    {
        free(m_content);
    }

    Samoyed::Terminal::Terminal &m_terminal;
    char *m_content;
    int m_length;
};

void output(GtkTextView *term, const char *content, int length)
{
    // TODO: Process escape sequences.
    gtk_text_buffer_insert_at_cursor(
        gtk_text_view_get_buffer(term),
        content, length);
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
    OutputParam *p = static_cast<OutputParam *>(param);
    if (!p->m_terminal.m_destroying)
        output(GTK_TEXT_VIEW(p->m_terminal.m_terminal),
               p->m_content, p->m_length);
    delete p;
    return FALSE;
}

void Terminal::onOutput(const char *content, int length)
{
    g_idle_add(onOutputInMainThread,
               new OutputParam(*this, content, length));
}

gboolean Terminal::onOutputDoneInMainThread(gpointer param)
{
    std::pair<Terminal *, char *> *p =
        static_cast<std::pair<Terminal *, char *> *>(param);
    delete p->first->m_outputReader;
    p->first->m_outputReader = NULL;
    if (!p->first->m_destroying && p->second)
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
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    if (p->first->m_destroying)
    {
        if (!p->first->m_inputWriter)
            delete p->first;
    }
    else
        p->first->view().close();
    g_free(p->second);
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
    if (!p->first->m_destroying && p->second)
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
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
    if (p->first->m_destroying)
    {
        if (!p->first->m_outputReader)
            delete p->first;
    }
    else if (p->second)
        p->first->view().close();
    else if (p->first->m_queuedInput)
    {
        p->first->m_inputWriter =
            new boost::thread(InputWriter(*p->first,
                                          p->first->m_queuedInput,
                                          p->first->m_queuedInputLength));
        free(p->first->m_queuedInput);
        p->first->m_queuedInput = NULL;
        p->first->m_queuedInputLength = 0;
    }
    g_free(p->second);
    delete p;
    return FALSE;
}

void Terminal::onInputDone(char *error)
{
    g_idle_add(onInputDoneInMainThread,
               new std::pair<Terminal *, char *>(this, error));
}

gboolean Terminal::onKeyPress(GtkWidget *widget,
                              GdkEventKey *event,
                              Terminal *term)
{
    char buffer[20];
    int length;
    guint32 key;
    switch (event->keyval)
    {
    case GDK_KEY_Return:
        buffer[0] = '\r';
        buffer[1] = '\n';
        length = 2;
        break;
    default:
        key = gdk_keyval_to_unicode(event->keyval);
        if (key == 0)
            length = 0;
        else
            length = g_unichar_to_utf8(key, buffer);
    }
    if (length > 0)
    {
        if (term->m_inputWriter)
        {
            term->m_queuedInput = static_cast<char *>(
            realloc(term->m_queuedInput, term->m_queuedInputLength + length));
            memcpy(term->m_queuedInput + term->m_queuedInputLength,
                   buffer,
                   sizeof(char) * length);
            term->m_queuedInputLength += length;
        }
        else
            term->m_inputWriter =
                new boost::thread(InputWriter(*term, buffer, length));
    }
    return TRUE;
}

#endif

GtkWidget *Terminal::setup()
{
#ifdef OS_WINDOWS
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
                       CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
                       NULL, NULL, &si, &pi))
    {
        error = _("Samoyed failed to start shell "
                  "\"C:\\Windows\\System32\\cmd.exe\".");
        goto ERROR_OUT;
    }
    m_process = pi.hProcess;
    m_processId = pi.dwProcessId;
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
    g_signal_connect(m_terminal, "key-press-event",
                     G_CALLBACK(onKeyPress), this);
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

void Terminal::destroy()
{
#ifdef OS_WINDOWS
    if (!m_outputReader && !m_inputWriter)
    {
        delete this;
        return;
    }
    m_destroying = true;
    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, m_processId);
#else
    delete this;
#endif
}

Terminal::Terminal(TerminalView &view):
    m_view(view)
#ifdef OS_WINDOWS
    ,
    m_process(INVALID_HANDLE_VALUE),
    m_outputReadHandle(INVALID_HANDLE_VALUE),
    m_inputWriteHandle(INVALID_HANDLE_VALUE),
    m_outputReader(NULL),
    m_inputWriter(NULL),
    m_queuedInput(NULL),
    m_queuedInputLength(0),
    m_destroying(false)
#endif
{
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
    free(m_queuedInput);
#endif
}

void Terminal::grabFocus()
{
    gtk_widget_grab_focus(m_terminal);
}

}

}
