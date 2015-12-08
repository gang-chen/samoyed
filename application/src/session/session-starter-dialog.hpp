// Session starter dialog.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SESSION_STARTER_DIALOG_HPP
#define SMYD_SESSION_STARTER_DIALOG_HPP

#include <string>
#include <boost/utility.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class SessionStarterDialog: public boost::noncopyable
{
public:
    enum Action
    {
        ACTION_CANCEL,
        ACTION_CREATE,
        ACTION_RESTORE
    };

    SessionStarterDialog(GtkWindow *parent);

    ~SessionStarterDialog();

    void run(Action &action, std::string &sessionName);

private:
    void buildNewSessionDialog();

    void readSessions();

    void buildRestoreSessionDialog();

    GtkWindow *m_parent;

    GtkBuilder *m_builder;

    GtkDialog *m_sessionStarterDialog;

    GtkDialog *m_newSessionDialog;
    GtkEntry *m_newSessionNameEntry;

    GtkDialog *m_restoreSessionDialog;
    GtkTreeView *m_sessionList;
};

}

#endif
