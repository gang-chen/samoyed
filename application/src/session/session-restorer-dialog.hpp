// Session restorer dialog.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SESSION_RESTORER_DIALOG_HPP
#define SMYD_SESSION_RESTORER_DIALOG_HPP

#include <list>
#include <string>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class SessionRestorerDialog: public boost::noncopyable
{
public:
    SessionRestorerDialog(const std::list<std::string> &sessionNames,
                          GtkWindow *parent);

    ~SessionRestorerDialog();

    bool run();

    boost::shared_ptr<char> sessionName() const;

private:
    void readSessions();

    static void validateInput(SessionRestorerDialog *dialog);

    GtkBuilder *m_builder;

    GtkDialog *m_dialog;

    GtkTreeView *m_sessionList;
};

}

#endif
