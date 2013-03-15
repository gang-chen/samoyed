// Notebook.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "notebook.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <string>
#include <vector>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define WIDGET "widget"
#define NOTEBOOK "notebook"
#define GROUP_NAME "group-name"
#define CREATE_CLOSE_BUTTONS "creaet-close-buttons"
#define CAN_DRAG_CHILDREN "can-drag-children"
#define USE_UNDERLINE "use-underline"
#define CHILDREN "children"
#define CURRENT_CHILD_INDEX "current-child-index"

namespace Samoyed
{

bool Notebook::XmlElement::registerReader()
{
    return Widget::XmlElement::registerReader(NOTEBOOK,
                                              Widget::XmlElement::Reader(read));
}

bool Notebook::XmlElement::readInternally(xmlDocPtr doc,
                                          xmlNodePtr node,
                                          std::list<std::string> &errors)
{
    char *value;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   WIDGET) == 0)
        {
            if (!WidgetContainer::XmlElement::readInternally(doc,
                                                             child, errors))
                return false;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        GROUP_NAME) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_groupName = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CREATE_CLOSE_BUTTONS) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_createCloseButtons = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CAN_DRAG_CHILDREN) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_canDragChildren = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        USE_UNDERLINE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_useUnderline = atoi(value);
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CHILDREN) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                Widget::XmlElement *ch =
                    Widget::XmlElement::read(doc, grandChild, errors);
                if (ch)
                    m_children.push_back(ch);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        CURRENT_CHILD_INDEX) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_currentChildIndex = atoi(value);
            xmlFree(value);
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
    return true;
}

Widget::XmlElement *Notebook::XmlElement::read(xmlDocPtr doc,
                                               xmlNodePtr node,
                                               std::list<std::string> &errors)
{
    XmlElement *element = new XmlElement;
    if (!element->readInternally(doc, node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr Notebook::XmlElement::write() const
{
    char *cp;
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(NOTEBOOK));
    xmlAddChild(node, WidgetContainer::XmlElement::write());
    if (groupName())
        xmlNewTextChild(node, NULL,
                        reinterpret_cast<const xmlChar *>(GROUP_NAME),
                        reinterpret_cast<const xmlChar *>(groupName()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CREATE_CLOSE_BUTTONS),
                    reinterpret_cast<const xmlChar *>(m_createCloseButtons ?
                                                      "1" : "0"));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CAN_DRAG_CHILDREN),
                    reinterpret_cast<const xmlChar *>(m_canDragChildren ?
                                                      "1" : "0"));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(USE_UNDERLINE),
                    reinterpret_cast<const xmlChar *>(m_useUnderline ?
                                                      "1" : "0"));
    xmlNodePtr children =
        xmlNewNode(NULL, reinterpret_cast<const xmlChar *>(CHILDREN));
    for (std::vector<Widget::XmlElement *>::const_iterator it =
             m_children.begin();
         it != m_children.end();
         ++it)
        xmlAddChild(children, (*it)->write());
    xmlAddChild(node, children);
    cp = g_strdup_printf("%d", m_currentChildIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CURRENT_CHILD_INDEX),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    return node;
}

void Notebook::XmlElement::saveWidgetInternally(const Notebook &notebook)
{
    WidgetContainer::XmlElement::saveWidgetInternally(notebook);
    if (notebook.groupName())
        m_groupName = notebook.groupName();
    m_createCloseButtons = notebook.createCloseButtons();
    m_canDragChildren = notebook.canDragChildren();
    m_useUnderline = notebook.useUnderline();
    m_children.reserve(notebook.childCount());
    for (int i = 0; i < notebook.childCount(); ++i)
        m_children.push_back(notebook.child(i).save());
    m_currentChildIndex = notebook.currentChildIndex();
}

Notebook::XmlElement *Notebook::XmlElement::saveWidget(const Notebook &notebook)
{
    XmlElement *element = new XmlElement;
    element->saveWidgetInternally(notebook);
    return element;
}

Widget *Notebook::XmlElement::restoreWidget()
{
    Notebook *notebook = new Notebook;
    if (!notebook->restore(*this))
    {
        delete notebook;
        return NULL;
    }
    return notebook;
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

bool Notebook::setup(const char *name, const char *groupName,
                     bool createCloseButtons,
                     bool canDragChildren,
                     bool useUnderline)
{
    if (!WidgetContainer::setup(name))
        return false;
    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    if (groupName)
        gtk_notebook_set_group_name(GTK_NOTEBOOK(notebook), groupName);
    m_createCloseButtons = createCloseButtons;
    m_canDragChildren = canDragChildren;
    m_useUnderline = useUnderline;
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
    gtk_widget_show_all(notebook);
    return true;
}

Notebook *Notebook::create(const char *name, const char *groupName,
                           bool createCloseButtons,
                           bool canDragChildren,
                           bool useUnderline)
{
    Notebook *notebook = new Notebook;
    if (!notebook->setup(name, groupName,
                         createCloseButtons,
                         canDragChildren,
                         useUnderline))
    {
        delete notebook;
        return NULL;
    }
    return notebook;
}

bool Notebook::restore(XmlElement &xmlElement)
{
    if (!WidgetContainer::restore(xmlElement))
        return false;
    GtkWidget *notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    if (xmlElement.groupName())
        gtk_notebook_set_group_name(GTK_NOTEBOOK(notebook),
                                    xmlElement.groupName());
    m_createCloseButtons = xmlElement.createCloseButtons();
    m_canDragChildren = xmlElement.canDragChildren();
    m_useUnderline = xmlElement.useUnderline();
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
    if (xmlElement.visible())
        gtk_widget_show_all(notebook);
    m_children.reserve(xmlElement.childCount());
    for (int i = 0; i < xmlElement.childCount(); ++i)
    {
        Widget *child = xmlElement.child(i).restoreWidget();
        if (!child)
        {
            xmlElement.removeChild(i);
            --i;
        }
        else
            addChild(*child, i);
    }
    if (childCount())
        setCurrentChildIndex(xmlElement.currentChildIndex());
    return true;
}

Notebook::~Notebook()
{
    assert(m_children.empty());
}

void Notebook::onCloseButtonClicked(GtkButton *button, Widget *child)
{
    child->close();
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
        if (static_cast<int>(i) != index)
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
    return XmlElement::saveWidget(*this);
}

void Notebook::addChildInternally(Widget &child, int index)
{
    // Create the tab label.
    GtkWidget *title;
    if (m_useUnderline)
        title = gtk_label_new_with_mnemonic(child.title());
    else
        title = gtk_label_new(child.title());
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
    gtk_widget_show(child.gtkWidget());
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
    int index = childIndex(oldChild);
    removeChildInternally(oldChild);
    addChildInternally(newChild, index);
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
                               Notebook *notebook)
{
    Widget *ch = Widget::getFromGtkWidget(child);
    assert(ch);
    notebook->m_children.erase(notebook->m_children.begin() +
                               notebook->childIndex(*ch));
    notebook->m_children.insert(notebook->m_children.begin() + index, ch);
}

void Notebook::onPageAdded(GtkWidget *widget, GtkWidget *child, int index,
                           Notebook *notebook)
{
    Widget *ch = Widget::getFromGtkWidget(child);
    assert(ch);
    notebook->WidgetContainer::addChildInternally(*ch);
    notebook->m_children.insert(notebook->m_children.begin() + index, ch);
}

void Notebook::onPageRemoved(GtkWidget *widget, GtkWidget *child, int index,
                             Notebook *notebook)
{
    Widget *ch = Widget::getFromGtkWidget(child);
    assert(ch);
    notebook->m_children.erase(notebook->m_children.begin() + index);
    notebook->WidgetContainer::removeChildInternally(*ch);
}

}
