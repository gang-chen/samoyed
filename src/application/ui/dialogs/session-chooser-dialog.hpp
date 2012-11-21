// Session chooser dialog.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SESSION_CHOOSER_DIALOG_HPP
#define SMYD_SESSION_CHOOSER_DIALOG_HPP

#include <boost/utility>
#include <gtk/gtk.h>

namespace Samoyed
{

class SessionChooserDialog: public boost::noncopyable
{
public:
    enum Response
    {
        CANCEL,
        CREATE,
        RESTORE
    };

    static SessionChooserDialog *create(bool choose);

    ~SessionChooserDialog();

    Response run();

    Response response() const;

    const char *sessionName() const;

    const char *newSessionName() const;

private:
    SessionChooserDialog();

    GtkWidget *m_dialog;
};

}

#endif
