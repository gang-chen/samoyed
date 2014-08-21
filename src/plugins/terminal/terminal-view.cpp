// View: terminal.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "terminal-view.hpp"
#include "application.hpp"
#include "ui/window.hpp"
#include "utilities/miscellaneous.hpp"
#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#ifndef G_OS_WIN32
# include <unistd.h>
# include <sys/types.h>
# include <pwd.h>
# include <vte/vte.h>
#endif

namespace
{

#ifndef G_OS_WIN32
void closeTerminal(VteTerminal *term, Samoyed::Terminal::TerminalView *termView)
{
    termView->close();
}
#endif

}

namespace Samoyed
{

namespace Terminal
{

bool TerminalView::setupTerminal()
{
#ifdef G_OS_WIN32
    m_terminal =
        gtk_label_new(_("Terminal is not supported on Windows platform."));
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
        return false;
    }
    free(argv[0]);
    g_signal_connect(m_terminal, "child-exited",
                     G_CALLBACK(closeTerminal), this);
#endif

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

}
