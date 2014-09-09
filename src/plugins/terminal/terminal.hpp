// Terminal.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_TERM_TERMINAL_HPP
#define SMYD_TERM_TERMINAL_HPP

#include <boost/utility.hpp>
#ifdef OS_WINDOWS
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
    Terminal(TerminalView &view):
        m_view(view)
#ifdef OS_WINDOWS
        ,
        m_process(INVALID_HANDLE_VALUE),
        m_outputRead(INVALID_HANDLE_VALUE),
        m_inputWrite(INVALID_HANDLE_VALUE),
        m_outputReader(NULL),
        m_inputWriter(NULL)
#endif
    {}

    ~Terminal();

    GtkWidget *setup();

    void grabFocus();

    TerminalView &view() { return m_view; }

private:
    TerminalView &m_view;

    GtkWidget *m_terminal;

#ifdef OS_WINDOWS
    HANDLE m_process;
    HANDLE m_outputRead;
    HANDLE m_inputWrite;
    boost::thread *m_outputReader;
    boost::thread *m_inputWriter;
#endif
};

}

}

#endif
