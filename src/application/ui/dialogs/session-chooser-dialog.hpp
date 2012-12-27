// Session chooser dialog.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SESSION_CHOOSER_DIALOG_HPP
#define SMYD_SESSION_CHOOSER_DIALOG_HPP

#include <string>
#include <boost/utility>
#include <gtk/gtk.h>

namespace Samoyed
{

class SessionChooserDialog: public boost::noncopyable
{
public:
    enum Action
    {
        ACTION_CANCEL,
        ACTION_CREATE,
        ACTION_RESTORE
    };

    SessionChooserDialog(Action action);

    ~SessionChooserDialog();

    void run();

    Action action() const { return m_action; }

    const char *sessionName() const { return m_sessionName.c_str(); }

private:
    GtkWidget *m_dialog;
    
    Action m_action;
    
    std::string m_sessionName;
};

}

#endif
