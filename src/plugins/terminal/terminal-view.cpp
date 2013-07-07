// View: terminal.
// Copyright (C) 2013 Gang Chen.

#include "terminal-view.hpp"
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <gtk/gtk.h>
#include <vte/vte.h>

namespace
{

void closeTerminal(VteTerminal *term, Samoyed::TerminalView *termView)
{
    termView->close();
}

}

namespace Samoyed
{

bool TerminalView::setupTerminal()
{
    m_terminal = vte_terminal_new();
    g_signal_connect(m_terminal, "child-exited",
                     G_CALLBACK(closeTerminal), this);

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
    if (!vte_terminal_fork_command_full(VTE_TERMINAL(m_terminal),
                                        VTE_PTY_DEFAULT,
                                        dir, argv, NULL,
                                        G_SPAWN_CHILD_INHERITS_STDIN,
                                        NULL, NULL, NULL, NULL))
    {
        free(argv[0]);
        return false;
    }
    free(argv[0]);

    GtkWidget *sb = gtk_scrollbar_new(
        GTK_ORIENTATION_VERTICAL,
        gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(m_terminal)));
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(box), m_terminal, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), sb, FALSE, TRUE, 0);
    setGtkWidget(box);

    return true;
}

bool TerminalView::setup(const char *id, const char *title)
{
    if (!View::setup(id))
        return false;
    setTitle(title);
    if (!setupTerminal())
        return false;
    gtk_widget_show_all(gtkWidget());
    return true;
}

bool TerminalView::restore(XmlElement &xmlElement)
{
    if (!View::restore(xmlElement))
        return false;
    if (!setupTerminal())
        return false;
    if (xmlElement.visible())
        gtk_widget_show_all(gtkWidget());
    return true;
}

TerminalView *TerminalView::create(const char *id, const char *title,
                                   const char *extensionId)
{
    TerminalView *view = new TerminalView(extensionId);
    if (!view->setup(id, title))
    {
        delete view;
        return NULL;
    }
    return view;
}

TerminalView *TerminalView::restore(XmlElement &xmlElement,
                                    const char *extensionId)
{
    TerminalView *view = new TerminalView(extensionId);
    if (!view->restore(xmlElement))
    {
        delete view;
        return NULL;
    }
    return view;
}

void TerminalView::grabFocus()
{
    gtk_widget_grab_focus(m_terminal);
}

}
