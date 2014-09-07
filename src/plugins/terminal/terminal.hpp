// Terminal.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_TERM_TERMINAL_HPP
#define SMYD_TERM_TERMINAL_HPP

#include <boost/utility.hpp>
#include <gtk/gtk.h>
#ifdef G_OS_WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

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
#ifdef G_OS_WIN32
        , m_process(INVALID_HANDLE_VALUE)
#endif
    {}

    ~Terminal();

    GtkWidget *setup();

    void grabFocus();

    TerminalView &view() { return m_view; }

private:
    TerminalView &m_view;

    GtkWidget *m_terminal;

#ifdef G_OS_WIN32
    HANDLE m_process;
#endif
};

}

}

#endif
