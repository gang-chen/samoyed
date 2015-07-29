// File recovery bar.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-recovery-bar.hpp"
#include "session.hpp"
#include "file-recoverers-extension-point.hpp"
#include "plugin/extension-point-manager.hpp"
#include "application.hpp"
#include <map>
#include <string>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#define FILE_RECOVERERS "file-recoverers"

namespace Samoyed
{

const char *FileRecoveryBar::ID = "file-recovery-bar";

FileRecoveryBar::FileRecoveryBar(
    const Session::UnsavedFileTable &files):
    m_files(files)
{
}

FileRecoveryBar::~FileRecoveryBar()
{
    g_object_unref(m_builder);
}

bool FileRecoveryBar::setup()
{
    if (!Bar::setup(ID))
        return false;

    std::string uiFile(Application::instance().dataDirectoryName());
    uiFile += G_DIR_SEPARATOR_S "ui" G_DIR_SEPARATOR_S
        "file-recovery-bar.xml";
    m_builder = gtk_builder_new_from_file(uiFile.c_str());

    GtkListStore *store =
        GTK_LIST_STORE(gtk_builder_get_object(m_builder, "files"));
    GtkTreeIter iter;
    for (Session::UnsavedFileTable::iterator it = m_files.begin();
         it != m_files.end();
         ++it)
    {
        gtk_list_store_append(store, &iter);
        gtk_list_store_set(store, &iter,
                           0, it->first.c_str(),
                           -1);
    }

    m_fileList = GTK_TREE_VIEW(gtk_builder_get_object(m_builder, "file-list"));

    g_signal_connect(gtk_builder_get_object(m_builder, "recover-button"),
                     "clicked", G_CALLBACK(onRecover), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "discard-button"),
                     "clicked", G_CALLBACK(onDiscard), this);
    g_signal_connect(gtk_builder_get_object(m_builder, "close-button"),
                     "clicked", G_CALLBACK(onClose), this);

    GtkWidget *grid = GTK_WIDGET(gtk_builder_get_object(m_builder, "grid"));
    setGtkWidget(grid);
    gtk_widget_show(grid);

    return true;
}

FileRecoveryBar *FileRecoveryBar::create(const Session::UnsavedFileTable &files)
{
    FileRecoveryBar *bar = new FileRecoveryBar(files);
    if (!bar->setup())
    {
        delete bar;
        return NULL;
    }
    return bar;
}

Widget::XmlElement *FileRecoveryBar::save() const
{
    return NULL;
}

void
FileRecoveryBar::setFiles(const Session::UnsavedFileTable &files)
{
    m_files = files;
    GtkTreeIter iter;
    GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(m_fileList));
    gtk_list_store_clear(store);
    for (Session::UnsavedFileTable::iterator it = m_files.begin();
         it != m_files.end();
         ++it)
        gtk_list_store_insert_with_values(store, &iter,
                                          0, it->first.c_str(),
                                          -1);
}

void FileRecoveryBar::onRecover(GtkButton *button, FileRecoveryBar *bar)
{
    GtkTreeSelection *select = gtk_tree_view_get_selection(bar->m_fileList);
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (gtk_tree_selection_get_selected(select, &model, &iter))
    {
        char *uri;
        gtk_tree_model_get(model, &iter, 0, &uri, -1);
        Session::UnsavedFileTable::iterator it = bar->m_files.find(uri);
        static_cast<FileRecoverersExtensionPoint &>(Application::instance().
            extensionPointManager().extensionPoint(FILE_RECOVERERS)).
            recoverFile(uri,
                        it->second.timeStamp,
                        it->second.mimeType.c_str(),
                        it->second.options);
        Application::instance().session().removeUnsavedFile(
            uri,
            it->second.timeStamp);
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
        bar->m_files.erase(it);
        g_free(uri);
        if (!gtk_tree_model_get_iter_first(model, &iter))
            bar->close();
    }
}

void FileRecoveryBar::onDiscard(GtkButton *button, FileRecoveryBar *bar)
{
    GtkTreeSelection *select = gtk_tree_view_get_selection(bar->m_fileList);
    GtkTreeIter iter;
    GtkTreeModel *model;
    if (gtk_tree_selection_get_selected(select, &model, &iter))
    {
        char *uri;
        gtk_tree_model_get(model, &iter, 0, &uri, -1);
        Session::UnsavedFileTable::iterator it = bar->m_files.find(uri);
        static_cast<FileRecoverersExtensionPoint &>(Application::instance().
            extensionPointManager().extensionPoint(FILE_RECOVERERS)).
            discardFile(uri, it->second.timeStamp, it->second.mimeType.c_str());
        Application::instance().session().removeUnsavedFile(
            uri,
            it->second.timeStamp);
        gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
        bar->m_files.erase(it);
        g_free(uri);
        if (!gtk_tree_model_get_iter_first(model, &iter))
            bar->close();
    }
}

void FileRecoveryBar::onClose(GtkButton *button, FileRecoveryBar *bar)
{
    bar->close();
}

}
