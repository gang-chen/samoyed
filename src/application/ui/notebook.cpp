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
    m_createCloseButtons(false),
    m_canDragChildren(false),
    m_currentChildIndex(0)
{
    char *value;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   "create-close-buttons") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_createCloseButtons = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        "can-drag-children") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_canDragChildren = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        "current-child-index") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
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
    if (m_currentChildIndex < 0)
    {
        if (childCount())
            m_currentChildIndex = 0;
        else
            m_currentChildIndex = -1;
    }
    else if (m_currentChildIndex == childCount())
        m_currentChildIndex = childCount() - 1;
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
    cp = g_strdup_printf("%d", m_createCloseButtons);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>("create-close-buttons"),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_canDragChildren);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>("can-drag-children"),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_currentChildIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>("current-child-index"),
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
    m_createCloseButtons = notebook.createCloseButtons();
    m_canDragChildren = notebook.canDragChildren();
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
    if (m_currentChildIndex > index)
        --m_currentChildIndex;
    else if (m_currentChildIndex == childCount())
        m_currentChildIndex = childCount() - 1;
}

Notebook::XmlElement::~XmlElement()
{
    for (std::vector<Widget::XmlElement *>::const_iterator it =
             m_children.begin();
         it != m_children.end();
         ++it)
        delete *it;
}

Notebook::Notebook(const char *name, const char *groupName,
                   bool createCloseButtons, bool canDragChildren):
    WidgetContainer(name),
    m_createCloseButtons(createCloseButtons),
    m_canDragChildren(canDragChildren)
{
    GtkWidget *notebook = gtk_notebook_new();
    if (groupName)
        gtk_notebook_set_group_name(GTK_NOTEBOOK(notebook), groupName);
    if (m_canDragChildren)
    {
        g_signal_connect(notebook, "page-reordered",
                         G_CALLBACK(onPageReordered), this);
        g_signal_connect(notebook, "page-added",
                         G_CALLBACK(onPageAdded), this);
        g_signal_connect(notebook, "page-removed",
                         G_CALLBACK(onPageRemoved), this);
    }
    setGtkWidget(notebook);
}

Notebook::Notebook(XmlElement &xmlElement):
    WidgetContainer(xmlElement),
    m_createCloseButtons(xmlElement.createCloseButtons()),
    m_canDragChildren(xmlElement.canDragChildren())
{
    GtkWidget *notebook = gtk_notebook_new();
    if (xmlElement.groupName())
        gtk_notebook_set_group_name(GTK_NOTEBOOK(notebook),
                                    xmlElement.groupName());
    if (m_canDragChildren)
    {
        g_signal_connect(notebook, "page-reordered",
                         G_CALLBACK(onPageReordered), this);
        g_signal_connect(notebook, "page-added",
                         G_CALLBACK(onPageAdded), this);
        g_signal_connect(notebook, "page-removed",
                         G_CALLBACK(onPageRemoved), this);
    }
    setGtkWidget(notebook);
    m_children.reserve(xmlElement.childCount());
    for (int i = 0; i < xmlElement.childCount(); ++i)
    {
        Widget *child = xmlElement.child(i).createWidget();
        if (!child)
            xmlElement.removeChild(i);
        else
            addChild(*child);
    }
    if (childCount())
        setCurrentChildIndex(xmlElement.currentChildIndex());
}

Notebook::~Notebook()
{
    assert(m_children.empty());
    gtk_widget_destroy(gtkWidget());
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
            if (!children[i]->close())
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

void Notebook::addChildInternally(Widget &child, int index)
{
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
    gtk_notebook_insert_page(GTK_NOTEBOOK(gtkWidget()),
                             child.gtkWidget(),
                             tabLabel,
                             index);

    if (m_canDragChildren)
    {
        gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(gtkWidget()),
                                         child.gtkWidget(),
                                         TRUE);
        gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(gtkWidget()),
                                        child.gtkWidget(),
                                        TRUE);
    }
}

void Notebook::removeChildInternally(Widget &child)
{
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTAINER(gtkWidget()), child.gtkWidget());
}

void Notebook::replaceChild(Widget &oldChild, Widget &newChild)
{
    int index = childIndex(&oldChild);
    removeChildInternally(oldChild);
    addChild(newChild, index);
}

void Notebook::onChildTitleChanged(const Widget &child)
{
    GtkWidget *tabLabel =
        gtk_notebook_get_tab_label(GTK_NOTEBOOK(gtkWidget()),
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
        gtk_notebook_get_tab_label(GTK_NOTEBOOK(gtkWidget()),
                                   child.gtkWidget());
    GtkWidget *title;
    if (m_createCloseButtons)
        title = gtk_grid_get_child_at(GTK_GRID(tabLabel), 0, 0);
    else
        title = tabLabel;
    gtk_widget_set_tooltip_text(title, child.description());
}

void Notebook::onPageReordered(GtkWidget *widget, GtkWidget *child, int index,
                               gpointer notebook)
{
    Notebook *nb = static_cast<Notebook *>(notebook);
    int oldIndex;
    Widget *ch;
    for (oldIndex = 0; oldIndex < childCount(); ++oldIndex)
        if (child(oldIndex).gtkWidget() == child)
        {
            ch = &child(oldIndex);
            break;
        }
    nb->m_children.erase(nb->m_children.begin() + oldIndex);
    nb->m_children.insert(nb->m_children.begin() + index, ch);
}

void Notebook::onPageAdded(GtkWidget *widget, GtkWidget *child, int index,
                           gpointer notebook)
{
    Notebook *nb = static_cast<Notebook *>(notebook);
    Widget *ch = Widget::getFromGtkWidget(child);
    nb->WidgetContainer::addChildInternally(*ch);
    m_children.insert(m_children.begin() + index, ch);
}

void Notebook::onPageRemoved(GtkWidget *widget, GtkWidget *child, int index,
                             gpointer notebook)
{
    Notebook *nb = static_cast<Notebook *>(notebook);
    Widget *ch = Widget::getFromGtkWidget(child);
    m_children.erase(m_children.begin() + childIndex(ch));
    nb->WidgetContainer::removeChildInternally(*ch);
}

}
