// Editor group.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "editor-group.hpp"
#include "editor.hpp"
#include <string>
#include <vector>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

namespace
{

const char *FILE_EDITED_MARK = "*";

}

namespace Samoyed
{

EditorGroup::EditorGroup(): Pane(TYPE_EDITOR_GROUP)
{
    m_notebook = gtk_notebook_new();
}

EditorGroup::~EditorGroup()
{
    assert(m_editors.empty());
    assert(m_titles.empty());
    gtk_widget_destroy(m_notebook);
}

void EditorGroup::onCloseButtonClicked(GtkButton *button, gpointer editor)
{
    static_cast<Editor *>(editor)->close();
}

bool EditorGroup::close()
{
    setClosing(true);
    if (m_editors.empty())
    {
        delete this;
        return true;
    }

    // First close the current editor and then close the others.
    std::vector<Editor *>::size_type currentIndex = currentEditorIndex();
    std::vector<Editor *> editors(m_editors);
    if (!m_editors[currentIndex]->close())
    {
        setClosing(false);
        return false;
    }
    for (std::vector<Editor *>::size_type i = 0; i < editors.size(); ++i)
    {
        if (i != currentIndex)
            if (!editors[i]->close())
            {
                setClosing(false);
                return false;
            }
    }
    return true;
}

void EditorGroup::addEditor(Editor &editor, int index)
{
    m_editors.insert(m_editors.begin() + index, &editor);
    for (std::vector<Editor *>::size_type i = index + 1;
         i < m_editors.size();
         ++i)
        m_editors[i]->setIndex(i);
    editor.addToGroup(*this, index);
    GtkWidget *title;
    if (editor.file().edited())
    {
        std::string s(FILE_EDITED_MARK);
        s += editor.file().name();
        title = gtk_label_new(s.c_str());
        s = editor.file().uri();
        s += _(" (edited)");
        gtk_widget_set_tooltip_text(title, s.c_str());
    }
    else
    {
        title = gtk_label_new(editor.file().name());
        gtk_widget_set_tooltip_text(title, editor.file().uri());
    }
    m_titles.insert(m_titles.begin() + index, title);
    GtkWidget *closeImage = gtk_image_new_from_stock(GTK_STOCK_CLOSE,
                                                     GTK_ICON_SIZE_MENU);
    GtkWidget *closeButton = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(closeButton), closeImage);
    gtk_widget_set_tooltip_text(closeButton, _("Close this editor"));
    g_signal_connect(closeButton, "clicked",
                     G_CALLBACK(onCloseButtonClicked), &editor);
    GtkWidget *tabLabel = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(tabLabel),
                            title, NULL,
                            GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(tabLabel),
                            closeButton, title,
                            GTK_POS_RIGHT, 1, 1);
    gtk_notebook_insert_page(GTK_NOTEBOOK(m_notebook),
                             editor.gtkWidget(),
                             tabLabel,
                             index);
}

void EditorGroup::removeEditor(Editor &editor)
{
    m_editors.erase(m_editors.begin() + editor.index());
    m_titles.erase(m_titles.begin() + editor.index());
    for (std::vector<Editor *>::size_type i = editor.index();
         i < m_editors.size();
         ++i)
        m_editors[i]->setIndex(i);
    editor.removeFromGroup();
}

void EditorGroup::onEditorClosed()
{
    if (closing() && m_editors.empty())
        delete this;
}

void EditorGroup::onEditorEditedStateChanged(Editor &editor)
{
    GtkWidget *title = m_titles[editor.index()];
    if (editor.file().edited())
    {
        std::string s(FILE_EDITED_MARK);
        s += editor.file().name();
        gtk_label_set_text(GTK_LABEL(title), s.c_str());
        s = editor.file().uri();
        s += _(" (edited)");
        gtk_widget_set_tooltip_text(title, s.c_str());
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(title), editor.file().name());
        gtk_widget_set_tooltip_text(title, editor.file().uri());
    }
}

}
