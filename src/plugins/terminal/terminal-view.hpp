// View: terminal.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TERM_TERMINAL_VIEW_HPP
#define SMYD_TERM_TERMINAL_VIEW_HPP

#include "widget/view.hpp"

namespace Samoyed
{

namespace Terminal
{

class Terminal;

class TerminalView: public View
{
public:
    static TerminalView *create(const char *id, const char *title,
                                const char *extensionId);

    static TerminalView *restore(XmlElement &xmlElement,
                                 const char *extensionId);

    virtual void grabFocus();

protected:
    TerminalView(const char *extensionId);

    virtual ~TerminalView();

    bool setupTerminal();

    bool setup(const char *id, const char *title);

    bool restore(XmlElement &xmlElement);

    Terminal *m_terminal;
};

}

}

#endif
