// Session management window.
// Copyright (C) 2014 Gang Chen.

#ifndef SMYD_SESSION_MANAGEMENT_WINDOW_HPP
#define SMYD_SESSION_MANAGEMENT_WINDOW_HPP

#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class SessionManagementWindow: public boost::noncopyable
{
public:
    SessionManagementWindow(
        GtkWindow *parent,
        const boost::function<void (SessionManagementWindow &)> &onDestroyed);
    ~SessionManagementWindow();

    void show();

    void hide();

private:
    void deleteSelectedSession(bool confirm);

    static void onRenameSession(GtkButton *button,
                                SessionManagementWindow *window);

    static void onDeleteSession(GtkButton *button,
                                SessionManagementWindow *window);

    static void onSessionRenamed(GtkCellRendererText *renderer,
                                 char *path,
                                 char *newName,
                                 SessionManagementWindow *window);

    static gboolean onKeyPress(GtkWidget *widget,
                               GdkEventKey *event,
                               SessionManagementWindow *window);

    static void onSessionSelectionChanged(GtkTreeSelection *selection,
                                          SessionManagementWindow *window);

    GtkBuilder *m_builder;
    GtkWindow *m_window;
    GtkTreeView *m_sessionList;

    boost::function<void (SessionManagementWindow &)> m_onDestroyed;
};

}

#endif
