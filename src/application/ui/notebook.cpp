// Notebook.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "notebook.hpp"
#include "widget.hpp"
#include <assert.h>
#include <stdlib.h>
#include <list>
#include <string>
#include <vector>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

bool Notebook::XmlElement::registerReader()
{
    return Widget::XmlElement::registerReader("notebook",
                                              Widget::XmlElement::Reader(read));
}

Notebook::XmlElement::XmlElement(xmlDocPtr doc,
                                 xmlNodePtr node,
                                 std::list<std::string> &errors):
    m_currentChildIndex(0)
{
    char *value;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   "current-child-index") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1);
            m_currentChildIndex = atoi(value);
            xmlFree(value);
        }
        else
        {
            Widget::XmlElement *ch =
                Widget::XmlElement::read(doc, child, errors);
            if (ch)
                m_children.push_back(ch);
        }
    }
}

Widget::XmlElement *Notebook::XmlElement::read(xmlDocPtr doc,
                                               xmlNodePtr node,
                                               std::list<std::string> &errors)
{
    return new XmlElement(doc, node, errors);
}

Notebook::XmlElement::XmlElement(const Notebook &notebook)
{
    m_children.reserve(notebook.childCount());
    for (int i = 0; i < notebook.childCount(); ++i)
        m_children.push_back(notebook.child(i).save());
    m_currentChildIndex = notebook.currentChildIndex();
}

Widget *Notebook::XmlElement::createWidget()
{
    return new Notebook(*this);
}

bool Notebook::XmlElement::restoreWidget(Widget &widget) const
{
    static_cast<Notebook &>(widget).setCurrentChildIndex(m_currentChildIndex);
}

Notebook::XmlElement::~XmlElement()
{
    for (std::vector<Widget::XmlElement *>::size_type i = 0;
         i < m_children.size();
         ++i)
        delete m_children[i];
}

Notebook::Notebook()
{
    m_notebook = gtk_notebook_new();
}

Notebook::Notebook(XmlElement& xmlElement)
{

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
    if (!children[index]->close())
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
