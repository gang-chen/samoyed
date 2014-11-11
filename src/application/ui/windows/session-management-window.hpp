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
        const boost::function<void (SessionManagementWindow &)> &onDestroy);
    ~SessionManagementWindow();

    void show();
    void hide();

private:
    static void onDeleteSession(GtkButton *button,
                                SessionManagementWindow *window);
    static void onRenameSession(GtkButton *button,
                                SessionManagementWindow *window);

    const char *sessionName() const;

    GtkWidget *m_window;
    GtkWidget *m_sessions;

    boost::function<void (SessionManagementWindow &)> &m_onClose;
};

}

#endif
