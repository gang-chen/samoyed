// Session restorer dialog.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "session-restorer-dialog.hpp"
#include "application.hpp"
#include <list>
#include <string>
#include <boost/shared_ptr.hpp>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

namespace
{

enum Column
{
    NAME_COLUMN,
    N_COLUMNS
};

void onSessionListRowActivated(GtkTreeView *list,
                               GtkTreeIter *iter,
                               GtkTreePath *path,
                               GtkWindow *dialog)
{
    gtk_window_activate_default(dialog);
}

}

namespace Samoyed
{

void SessionRestorerDialog::validateInput(SessionRestorerDialog *dialog)
{
    gtk_dialog_set_response_sensitive(
        dialog->m_dialog,
        GTK_RESPONSE_ACCEPT,
        gtk_tree_selection_get_selected(
            gtk_tree_view_get_selection(dialog->m_sessionList),
            NULL,
            NULL));
}

SessionRestorerDialog::SessionRestorerDialog(
    const std::list<std::string> &sessionNames,
    GtkWindow *parent)
{
    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "session-restorer-dialog.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());

    m_dialog =
        GTK_DIALOG(gtk_builder_get_object(m_builder,
                                          "session-restorer-dialog"));
    if (parent)
    {
        gtk_window_set_transient_for(GTK_WINDOW(m_dialog),
                                     parent);
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    }

    GtkListStore *store =
        GTK_LIST_STORE(gtk_builder_get_object(m_builder, "sessions"));
    GtkTreeIter treeIt;
    for (std::list<std::string>::const_iterator it = sessionNames.begin();
         it != sessionNames.end();
         ++it)
    {
        gtk_list_store_append(store, &treeIt);
        gtk_list_store_set(store, &treeIt, NAME_COLUMN, it->c_str(), -1);
    }

    m_sessionList =
        GTK_TREE_VIEW(gtk_builder_get_object(m_builder, "session-list"));
    g_signal_connect_swapped(gtk_tree_view_get_selection(m_sessionList),
                             "changed",
                             G_CALLBACK(validateInput),
                             this);
    g_signal_connect(m_sessionList, "row-activated",
                     G_CALLBACK(onSessionListRowActivated),
                     GTK_WINDOW(m_dialog));

    validateInput(this);
}

SessionRestorerDialog::~SessionRestorerDialog()
{
    gtk_widget_destroy(GTK_WIDGET(m_dialog));
    g_object_unref(m_builder);
}

bool SessionRestorerDialog::run()
{
    return gtk_dialog_run(m_dialog) == GTK_RESPONSE_ACCEPT;
}

boost::shared_ptr<char> SessionRestorerDialog::sessionName() const
{
    GtkTreeSelection *selection;
    GtkTreeModel *model;
    GtkTreeIter it;
    char *sessionName;
    selection = gtk_tree_view_get_selection(m_sessionList);
    if (gtk_tree_selection_get_selected(selection, &model, &it))
    {
        gtk_tree_model_get(model, &it, NAME_COLUMN, &sessionName, -1);
        return boost::shared_ptr<char>(sessionName, g_free);
    }
    return boost::shared_ptr<char>();
}

}
