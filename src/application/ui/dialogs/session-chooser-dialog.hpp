// Session chooser dialog.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SESSION_CHOOSER_DIALOG_HPP
#define SMYD_SESSION_CHOOSER_DIALOG_HPP

#include <string>
#include <boost/utility.hpp>
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
    class Dialog: public boost::noncopyable
    {
    public:
        virtual ~Dialog() {}
        virtual void run() = 0;
        virtual const char *name() const = 0;
    };

    class NewSessionDialog: public Dialog
    {
    public:
        NewSessionDialog();
        virtual ~NewSessionDialog();
        virtual void run();
        virtual const char *name() const;

    private:
        static void onResponse(GtkDialog *dialog, int id, gpointer d);
        GtkWidget *m_name;
        GtkWidget *m_dialog;
    };

    class RestoreSessionDialog: public WorkerDialog
    {
    public:
        RestoreSessionDialog();
        virtual ~RestoreSessionDialog();
        virtual void run();
        virtual const char *name() const;

    private:
        static void onResponse(GtkDialog *dialog, int id, gpointer d);
        GtkWidget *m_list;
        GtkWidget *m_dialog;
    }

    Dialog *m_dialog;

    Action m_action;

    std::string m_sessionName;
};

}

#endif
