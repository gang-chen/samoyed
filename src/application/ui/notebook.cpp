// Notebook.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "notebook.hpp"
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
    m_currentChildIndex(0),
    m_closeButtonsExist(true)
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
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   "close-buttons-exist") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1);
            m_closeButtonsExist = atoi(value);
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

xmlNodePtr Notebook::XmlElement::write() const
{
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>("notebook"));
    char *cp;
    cp = g_strdup_printf("%d", m_currentChildIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>("current-child-index"),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_closeButtonsExist);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>("close-buttons-exist"),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    for (std::vector<Widget::XmlElement *>::const_iterator it =
             m_children.begin();
         it != m_children.end();
         ++it)
        xmlAddChild(node, (*it)->write());
    return node;
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

void Notebook::XmlElement::removeChild(int index)
{
    delete m_children[index];
    m_children.erase(m_children.begin() + index);
}

Notebook::XmlElement::~XmlElement()
{
    for (std::vector<Widget::XmlElement *>::const_iterator it =
             m_children.begin();
         it != m_children.end();
         ++it)
        delete *it;
}

Notebook::Notebook(const char *name, bool createCloseButtons):
    WidgetContainer(name),
    m_createCloseButtons(createCloseButtons)
{
    m_notebook = gtk_notebook_new();
}

Notebook::Notebook(XmlElement &xmlElement):
    WidgetContainer(xmlElement)
{
    m_notebook = gtk_notebook_new();
    m_children.reserve(xmlElement.childCount());
    for (int i = 0; i < xmlElement.childCount(); ++i)
    {
        Widget *child = xmlElement.child(i).createWidget();
        if (!child)
            xmlElement.removeChild(i);
        else
            addChild(*child);
    }
    setCurrentChildIndex(xmlElement.currentChildIndex());
    m_createCloseButtons = xmlElement.closeButtonsExist();
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

Widget::XmlElement *Notebook::save() const
{
    return new XmlElement(*this);
}

void Notebook::onChildClosed(const Widget &child)
{
    m_children.erase(m_children.begin() + childIndex(child));
    WidgetContainer::onChildClosed(child);
}

void Notebook::addChild(Widget &child, int index)
{
    assert(!child.parent());
    WidgetContainer::addChild(child);
    m_children.insert(m_children.begin() + index, &child);
    child.setParent(this);

    // Create the tab label.
    GtkWidget *title = gtk_label_new(child.title());
    gtk_widget_set_tooltip_text(title, child.description());
    GtkWidget *tabLabel;
    if (m_createCloseButtons)
    {
        GtkWidget *closeImage = gtk_image_new_from_stock(GTK_STOCK_CLOSE,
                                                         GTK_ICON_SIZE_MENU);
        GtkWidget *closeButton = gtk_button_new();
        gtk_container_add(GTK_CONTAINER(closeButton), closeImage);
        gtk_widget_set_tooltip_text(closeButton, _("Close this editor"));
        g_signal_connect(closeButton, "clicked",
                         G_CALLBACK(onCloseButtonClicked), &child);
        tabLabel = gtk_grid_new();
        gtk_grid_attach_next_to(GTK_GRID(tabLabel),
                                title, NULL,
                                GTK_POS_RIGHT, 1, 1);
        gtk_grid_attach_next_to(GTK_GRID(tabLabel),
                                closeButton, title,
                                GTK_POS_RIGHT, 1, 1);
    }
    else
        tabLabel = title;

    // Show the child widget before adding it to the notebook.
    gtk_widget_show(child->gtkWidget());
    gtk_notebook_insert_page(GTK_NOTEBOOK(m_notebook),
                             child.gtkWidget(),
                             tabLabel,
                             index);
}

void Notebook::removeChild(Widget &child)
{
    assert(child.parent() == this);
    WidgetContainer::removeChild(child);
    m_children.erase(m_children.begin() + childIndex(&child));
    child.setParent(NULL);
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTAINER(m_notebook), child.gtkWidget());
}

void Notebook::replaceChild(Widget &oldChild, Widget &newChild)
{
    int index = childIndex(&oldChild);
    removeChild(oldChild);
    addChild(newChild, index);
}

void Notebook::onChildTitleChanged(const Widget &child)
{
    GtkWidget *tabLabel =
        gtk_notebook_get_tab_label(GTK_NOTEBOOK(m_notebook),
                                   child.gtkWidget());
    GtkWidget *title;
    if (m_createCloseButtons)
        title = gtk_grid_get_child_at(GTK_GRID(tabLabel), 0, 0);
    else
        title = tabLabel;
    gtk_label_set_text(GTK_LABEL(title), child.title());
}

void Notebook::onChildDescriptionChanged(const Widget &child)
{
    GtkWidget *tabLabel =
        gtk_notebook_get_tab_label(GTK_NOTEBOOK(m_notebook),
                                   child.gtkWidget());
    GtkWidget *title;
    if (m_createCloseButtons)
        title = gtk_grid_get_child_at(GTK_GRID(tabLabel), 0, 0);
    else
        title = tabLabel;
    gtk_widget_set_tooltip_text(title, child.description());
}

}
