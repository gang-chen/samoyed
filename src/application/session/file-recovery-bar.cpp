// File recovery bar.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-recovery-bar.hpp"
#include "session.hpp"
#include "editors/file-recoverers-extension-point.hpp"
#include "plugin/extension-point-manager.hpp"
#include "utilities/property-tree.hpp"
#include "utilities/miscellaneous.hpp"
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
}

bool FileRecoveryBar::setup()
{
    if (!Bar::setup(ID))
        return false;
    GtkListStore *store = gtk_list_store_new(1, G_TYPE_STRING);
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
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    m_fileList = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(m_fileList), FALSE);
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("URIs"),
                                                      renderer,
                                                      "text", 0,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(m_fileList), column);
    GtkTreeSelection *select =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(m_fileList));
    gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);
    gtk_widget_set_hexpand(m_fileList, TRUE);
    gtk_widget_set_vexpand(m_fileList, TRUE);

    GtkWidget *grid = gtk_grid_new();
    GtkWidget *label = gtk_label_new_with_mnemonic(
        _("_Files that were edited but not saved in the last session:"));
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), m_fileList);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), m_fileList, 0, 1, 1, 1);

    GtkWidget *button;
    GtkWidget *box = gtk_button_box_new(GTK_ORIENTATION_VERTICAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(box), GTK_BUTTONBOX_START);
    gtk_box_set_spacing(GTK_BOX(box), CONTAINER_SPACING);
    gtk_box_set_homogeneous(GTK_BOX(box), TRUE);
    button = gtk_button_new_with_mnemonic(_("_Recover"));
    gtk_widget_set_tooltip_text(
        button, _("Open the file and replay the saved edits"));
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
    g_signal_connect(button, "clicked", G_CALLBACK(onRecover), this);
    button = gtk_button_new_with_mnemonic(_("_Discard"));
    gtk_widget_set_tooltip_text(
        button,
        _("Discard the edits and delete the replay file"));
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
    g_signal_connect(button, "clicked", G_CALLBACK(onDiscard), this);
    button = gtk_button_new_with_mnemonic(_("_Close"));
    gtk_widget_set_tooltip_text(button, _("Close this bar"));
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
    gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(box), button, TRUE);
    g_signal_connect(button, "clicked", G_CALLBACK(onClose), this);
    gtk_grid_attach(GTK_GRID(grid), box, 1, 0, 1, 2);

    gtk_grid_set_row_spacing(GTK_GRID(grid), CONTAINER_SPACING);
    gtk_grid_set_column_spacing(GTK_GRID(grid), CONTAINER_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(grid), CONTAINER_BORDER_WIDTH);
    gtk_widget_set_vexpand(grid, FALSE);

    setGtkWidget(grid);
    gtk_widget_show_all(grid);

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
    GtkListStore *store =
        GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(m_fileList)));
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
    GtkTreeSelection *select =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(bar->m_fileList));
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
    GtkTreeSelection *select =
        gtk_tree_view_get_selection(GTK_TREE_VIEW(bar->m_fileList));
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
