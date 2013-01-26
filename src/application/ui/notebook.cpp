// Notebook.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "notebook.hpp"
#include <gtk/gtk.h>
#include <libxml/xmlstring.h>

namespace Samoyed
{

Notebook::Notebook()
{
    m_notebook = gtk_notebook_new();
}

Notebook::~Notebook()
{
    assert(m_children.empty());
    gtk_widget_destroy(m_notebook);
}

void Notebook::onCloseButtonClicked(GtkButton *button, gpointer child)
{
    static_cast<Widget *>(child)->close();
}

bool Notebook::close()
{
    if (closing())
        return true;

    setClosing(true);
    if (m_children.empty())
    {
        delete this;
        return true;
    }

    // First close the current child and then close the others.  Copy the
    // children to a temporary vector because the original vector will be
    // changed if any child is closed.
    int index = currentChildIndex();
    std::vector<Widget *> children(m_children);
    if (!children[currentChildIndex]->close())
    {
        setClosing(false);
        return false;
    }
    for (std::vector<Widget *>::size_type i = 0; i < children.size(); ++i)
    {
        if (i != index)
            if (!editors[i]->close())
            {
                setClosing(false);
                return false;
            }
    }
    return true;
}

void Notebook::addChild(Widget &child, int index)
{
    m_children.insert(m_children.begin() + index, &child);

    // Create the tab label.
    GtkWidget *title = gtk_label_new(child.title());
    gtk_widget_set_tooltip_text(title, child.description());
    GtkWidget *closeImage = gtk_image_new_from_stock(GTK_STOCK_CLOSE,
                                                     GTK_ICON_SIZE_MENU);
    GtkWidget *closeButton = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(closeButton), closeImage);
    gtk_widget_set_tooltip_text(closeButton, _("Close this editor"));
    g_signal_connect(closeButton, "clicked",
                     G_CALLBACK(onCloseButtonClicked), &child);
    GtkWidget *tabLabel = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(tabLabel),
                            title, NULL,
                            GTK_POS_RIGHT, 1, 1);
    gtk_grid_attach_next_to(GTK_GRID(tabLabel),
                            closeButton, title,
                            GTK_POS_RIGHT, 1, 1);
    gtk_notebook_insert_page(GTK_NOTEBOOK(m_notebook),
                             child.gtkWidget(),
                             tabLabel,
                             index);
}

void Notebook::removeChild(Widget &child)
{
    m_children.erase(m_children.begin() + childIndex(&child));
    gtk_container_remove(GTK_CONTAINER(m_notebook), child.gtkWidget());
}

void Notebook::onChildClosed(Widget *child)
{
    m_children.erase(m_children.begin() + childIndex(child));
    if (closing() && m_children.empty())
        delete this;
}

int Notebook::childIndex(const Widget *child) const
{
    for (std::vector<Widget *>::size_type i = 0; i < m_children.size(); ++i)
        if (m_children[i] == child)
            return i;
    return -1;
}

void Notebook::onChildTitleChanged(Widget &child)
{
    GtkWidget *tabLabel =
        gtk_notebook_get_tab_label(GTK_NOTEBOOK(m_notebook),
                                   child.gtkWidget());
    GtkLabel *title = gtk_grid_get_child_at(GTK_GRID(tabLabel), 0, 0);
    gtk_label_set_text(GTK_LABEL(title), child.title());
    gtk_widget_set_tooltip_text(title, child.description());
}

}
