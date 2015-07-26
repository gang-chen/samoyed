// Terminal.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_TERM_TERMINAL_HPP
#define SMYD_TERM_TERMINAL_HPP

#include <boost/utility.hpp>
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

private:
    ~Terminal();

    TerminalView &m_view;

    GtkWidget *m_terminal;
};

}

}

#endif
