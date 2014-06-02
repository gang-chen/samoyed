// File recovery bar.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "file-recovery-bar.hpp"
#include "utilities/property-tree.hpp"
#include "utilities/miscellaneous.hpp"
#include <map>
#include <string>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

namespace Samoyed
{

const char *FileRecoveryBar::ID = "file-recovery-bar";

FileRecoveryBar::FileRecoveryBar(
    const std::map<std::string, PropertyTree *> &files):
    m_files(files),
    m_store(NULL)
{
    for (std::map<std::string, PropertyTree *>::iterator it = m_files.begin();
         it != m_files.end();
         ++it)
        it->second = new PropertyTree(*it->second);
}

FileRecoveryBar::~FileRecoveryBar()
{
    if (m_store)
        g_object_unref(m_store);
    for (std::map<std::string, PropertyTree *>::iterator it = m_files.begin();
         it != m_files.end();
         ++it)
        delete it->second;
}

bool FileRecoveryBar::setup()
{
    if (!Bar::setup(ID))
        return false;
    m_store = gtk_list_store_new(1, G_TYPE_STRING);
    GtkTreeIter it;
    for (std::map<std::string, PropertyTree *>::iterator i = m_files.begin();
         i != m_files.end();
         ++i)
    {
        gtk_list_store_append(m_store, &it);
        gtk_list_store_set(m_store, &it,
                           0, i->first.c_str(),
                           -1);
    }
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkWidget *list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(m_store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(list), FALSE);
    renderer = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(_("URIs"),
                                                      renderer,
                                                      "text", 0,
                                                      NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
    gtk_widget_set_hexpand(list, TRUE);

    GtkWidget *grid = gtk_grid_new();
    GtkWidget *label = gtk_label_new_with_mnemonic(
        _("_Files that were edited but not saved in the last session:"));
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), list);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), list, 0, 1, 1, 1);
    gtk_grid_set_row_spacing(GTK_GRID(grid), CONTAINER_SPACING);

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
    button = gtk_button_new_with_mnemonic(_("_Show differences"));
    gtk_widget_set_tooltip_text(
        button,
        _("Show the differences between the saved content and the edited "
          "content"));
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, TRUE, 0);
    g_signal_connect(button, "clicked", G_CALLBACK(onShowDifferences), this);
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
    gtk_grid_set_column_spacing(GTK_GRID(grid), CONTAINER_SPACING);
    gtk_container_set_border_width(GTK_CONTAINER(grid), CONTAINER_BORDER_WIDTH);

    int total_h, label_h;
    gtk_widget_get_preferred_height(box, &total_h, NULL);
    gtk_widget_get_preferred_height(label, &label_h, NULL);
    gtk_widget_set_size_request(list, -1,
                                total_h - label_h - CONTAINER_SPACING);

    setGtkWidget(grid);
    gtk_widget_show_all(grid);

    return true;
}

FileRecoveryBar *
FileRecoveryBar::create(const std::map<std::string, PropertyTree *> &files)
{
    FileRecoveryBar *bar = new FileRecoveryBar(files);
    if (!bar->setup())
    {
        delete bar;
        return NULL;
    }
    return bar;
}

bool FileRecoveryBar::close()
{
    delete this;
    return true;
}

Widget::XmlElement *FileRecoveryBar::save() const
{
    return NULL;
}

void
FileRecoveryBar::setFiles(const std::map<std::string, PropertyTree *> &files)
{
    for (std::map<std::string, PropertyTree *>::iterator it = m_files.begin();
         it != m_files.end();
         ++it)
        delete it->second;
    m_files = files;
    for (std::map<std::string, PropertyTree *>::iterator it = m_files.begin();
         it != m_files.end();
         ++it)
        it->second = new PropertyTree(*it->second);
    GtkTreeIter it;
    gtk_list_store_clear(m_store);
    for (std::map<std::string, PropertyTree *>::iterator i = m_files.begin();
         i != m_files.end();
         ++i)
        gtk_list_store_insert_with_values(m_store, &it,
                                          0, i->first.c_str(),
                                          -1);
}

void FileRecoveryBar::onRecover(GtkButton *button, FileRecoveryBar *bar)
{
}

void FileRecoveryBar::onShowDifferences(GtkButton *button, FileRecoveryBar *bar)
{
}

void FileRecoveryBar::onDiscard(GtkButton *button, FileRecoveryBar *bar)
{
}

void FileRecoveryBar::onClose(GtkButton *button, FileRecoveryBar *bar)
{
    bar->close();
}

}
