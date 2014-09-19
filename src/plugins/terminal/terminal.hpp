// Terminal.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_TERM_TERMINAL_HPP
#define SMYD_TERM_TERMINAL_HPP

#include <boost/utility.hpp>
#ifdef OS_WINDOWS
# include <boost/thread.hpp>
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif
#include <gtk/gtk.h>

namespace Samoyed
{

namespace Terminal
{

class TerminalView;

class Terminal: public boost::noncopyable
{
public:
    Terminal(TerminalView &view);

    void destroy();

    GtkWidget *setup();

    void grabFocus();

    TerminalView &view() { return m_view; }

#ifdef OS_WINDOWS
    HANDLE outputReadHandle() { return m_outputReadHandle; }
    HANDLE inputWriteHandle() { return m_inputWriteHandle; }

    void onOutput(const char *content, int length);
    void onOutputDone(char *error);
    void onInputDone(char *error);
#endif

private:
    ~Terminal();

#ifdef OS_WINDOWS
    static gboolean onOutputInMainThread(gpointer param);
    static gboolean onOutputDoneInMainThread(gpointer param);
    static gboolean onInputDoneInMainThread(gpointer param);
    static gboolean onKeyPress(GtkWidget *widget,
                               GdkEventKey *event,
                               Terminal *term);
#endif

    TerminalView &m_view;

    GtkWidget *m_terminal;

#ifdef OS_WINDOWS
    DWORD m_processId;
    HANDLE m_process;
    HANDLE m_outputReadHandle;
    HANDLE m_inputWriteHandle;
    boost::thread *m_outputReader;
    boost::thread *m_inputWriter;
    char *m_queuedInput;
    int m_queuedInputLength;
    bool m_destroying;
#endif
};

}

}

#endif
