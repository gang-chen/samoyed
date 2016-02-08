// Session creator dialog.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SESSION_CREATOR_DIALOG_HPP
#define SMYD_SESSION_CREATOR_DIALOG_HPP

#include <string>
#include <boost/utility.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class SessionCreatorDialog: public boost::noncopyable
{
public:
    SessionCreatorDialog(GtkWindow *parent);

    ~SessionCreatorDialog();

    bool run();

    const char *sessionName() const;

private:
    static void validateInput(SessionCreatorDialog *dialog);

    GtkBuilder *m_builder;

    GtkDialog *m_dialog;

    GtkEntry *m_sessionNameEntry;
};

}

#endif
