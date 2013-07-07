// View: terminal.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TERMINAL_VIEW_HPP
#define SMYD_TERMINAL_VIEW_HPP

#include "ui/view.hpp"

namespace Samoyed
{

class TerminalView: public View
{
public:
    static TerminalView *create(const char *id, const char *title,
                                const char *extensionId);

    static TerminalView *restore(XmlElement &xmlElement,
                                 const char *extensionId);

    virtual void grabFocus();

protected:
    TerminalView(const char *extensionId): View(extensionId), m_terminal(NULL)
    {}

    bool setupTerminal();

    bool setup(const char *id, const char *title);

    bool restore(XmlElement &xmlElement);

    GtkWidget *m_terminal;
};

}

#endif
