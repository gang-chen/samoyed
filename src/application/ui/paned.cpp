// Paned widget.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "paned.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <string>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

#define WIDGET_CONTAINER "widget-container"
#define PANED "paned"
#define ORIENTATION "orientation"
#define HORIZONTAL "horizontal"
#define VERTICAL "vertical"
#define CHILDREN "children"
#define CURRENT_CHILD_INDEX "current-child-index"
#define POSITION "position"

namespace Samoyed
{

bool Paned::XmlElement::registerReader()
{
    return Widget::XmlElement::registerReader(PANED,
                                              Widget::XmlElement::Reader(read));
}

bool Paned::XmlElement::readInternally(xmlDocPtr doc,
                                       xmlNodePtr node,
                                       std::list<std::string> &errors)
{
    char *value, *cp;
    bool containerSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   WIDGET_CONTAINER) == 0)
        {
            if (containerSeen)
            {
                cp = g_strdup_printf(
                    _("Line %d: More than one \"%s\" elements seen.\n"),
                    child->line, WIDGET_CONTAINER);
                errors.push_back(cp);
                g_free(cp);
                return false;
            }
            if (!WidgetContainer::XmlElement::readInternally(doc, child,
                                                             errors))
                return false;
            containerSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        ORIENTATION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (strcmp(value, HORIZONTAL) == 0)
                m_orientation = Paned::ORIENTATION_HORIZONTAL;
            else if (strcmp(value, VERTICAL) == 0)
                m_orientation = Paned::ORIENTATION_VERTICAL;
            else
            {
                cp = g_strdup_printf(
                    _("Line %d: Unknown orientation \"%s\".\n"),
                    child->line, value);
                errors.push_back(cp);
                g_free(cp);
            }
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
                {
                    if (m_children[1])
                    {
                        cp = g_strdup_printf(
                            _("Line %d: More than two children contained by "
                              "the paned widget.\n"),
                            grandChild->line);
                        errors.push_back(cp);
                        g_free(cp);
                        delete ch;
                    }
                    else if (m_children[0])
                        m_children[1] = ch;
                    else
                        m_children[0] = ch;
                }
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
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        POSITION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_position = atoi(value);
            xmlFree(value);
        }
    }

    if (!containerSeen)
    {
        cp = g_strdup_printf(
            _("Line %d: \"%s\" element missing.\n"),
            node->line, WIDGET_CONTAINER);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    if (!m_children[0])
    {
        cp = g_strdup_printf(
            _("Line %d: No child contained by the paned widget.\n"),
            node->line);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    if (!m_children[1])
    {
        cp = g_strdup_printf(
            _("Line %d: Only one child contained by the paned widget.\n"),
            node->line);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }

    if (m_currentChildIndex < 0)
        m_currentChildIndex = 0;
    else if (m_currentChildIndex > 1)
        m_currentChildIndex = 1;
    return true;
}

Paned::XmlElement *Paned::XmlElement::read(xmlDocPtr doc,
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

xmlNodePtr Paned::XmlElement::write() const
{
    char *cp;
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(PANED));
    xmlAddChild(node, WidgetContainer::XmlElement::write());
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(ORIENTATION),
                    m_orientation ==
                    Paned::ORIENTATION_HORIZONTAL ?
                    reinterpret_cast<const xmlChar *>(HORIZONTAL) :
                    reinterpret_cast<const xmlChar *>(VERTICAL));
    xmlNodePtr children = xmlNewNode(NULL,
                                     reinterpret_cast<const xmlChar *>(CHILDREN));
    xmlAddChild(children, m_children[0]->write());
    xmlAddChild(children, m_children[1]->write());
    xmlAddChild(node, children);
    cp = g_strdup_printf("%d", m_currentChildIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CURRENT_CHILD_INDEX),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_position);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(POSITION),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    return node;
}

Paned::XmlElement::XmlElement(const Paned &paned):
    WidgetContainer::XmlElement(paned)
{
    m_orientation = paned.orientation();
    m_children[0] = paned.child(0).save();
    m_children[1] = paned.child(1).save();
    m_currentChildIndex = paned.currentChildIndex();
    m_position = paned.position();
}

Widget *Paned::XmlElement::restoreWidget()
{
    Paned *paned = new Paned;
    if (!paned->restore(*this))
    {
        delete paned;
        return NULL;
    }
    return paned;
}

Paned::XmlElement::~XmlElement()
{
    delete m_children[0];
    delete m_children[1];
}

bool Paned::setup(const char *name,
                  Orientation orientation,
                  Widget &child1, Widget &child2)
{
    if (!WidgetContainer::setup(name))
        return false;
    GtkWidget *paned = gtk_paned_new(static_cast<GtkOrientation>(orientation));
    g_signal_connect(paned, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(paned);
    gtk_widget_show_all(paned);
    addChildInternally(child1, 0);
    addChildInternally(child2, 1);
    return true;
}

Paned *Paned::create(const char *name,
                     Orientation orientation,
                     Widget &child1, Widget &child2)
{
    Paned *paned = new Paned;
    if (!paned->setup(name, orientation, child1, child2))
    {
        delete paned;
        return NULL;
    }
    return paned;
}

bool Paned::restore(XmlElement &xmlElement)
{
    if (!WidgetContainer::restore(xmlElement))
        return false;
    Widget *child1 = xmlElement.child(0).restoreWidget();
    if (!child1)
        return false;
    Widget *child2 = xmlElement.child(1).restoreWidget();
    if (!child2)
    {
        // Assume that it should close itself silently.
        child1->close();
        return false;
    }
    GtkWidget *paned =
        gtk_paned_new(static_cast<GtkOrientation>(xmlElement.orientation()));
    g_signal_connect(paned, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(paned);
    if (xmlElement.visible())
        gtk_widget_show_all(paned);
    addChildInternally(*child1, 0);
    addChildInternally(*child2, 1);
    setCurrentChildIndex(xmlElement.currentChildIndex());
    if (xmlElement.position() >= 0)
        setPosition(xmlElement.position());
    return true;
}

Paned::~Paned()
{
    assert(!m_children[0] && !m_children[1]);
}

bool Paned::close()
{
    if (closing())
        return true;

    setClosing(true);
    Widget *child1 = m_children[m_currentChildIndex];
    Widget *child2 = m_children[1 - m_currentChildIndex];
    if (!child1->close())
    {
        setClosing(false);
        return false;
    }
    if (!child2->close())
        // Do not call setClosing(false) because the paned widget will be
        // deleted if either child is closed.
        return false;
    return true;
}

Widget::XmlElement *Paned::save() const
{
    return new XmlElement(*this);
}

void Paned::addChildInternally(Widget &child, int index)
{
    assert(!m_children[index]);
    WidgetContainer::addChildInternally(child);
    m_children[index] = &child;
    if (index == 0)
        gtk_paned_pack1(GTK_PANED(gtkWidget()), child.gtkWidget(), TRUE, TRUE);
    else
        gtk_paned_pack2(GTK_PANED(gtkWidget()), child.gtkWidget(), TRUE, TRUE);
}

void Paned::removeChildInternally(Widget &child)
{
    int index = childIndex(child);
    m_children[index] = NULL;
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTAINER(gtkWidget()), child.gtkWidget());
    WidgetContainer::removeChildInternally(child);
}

void Paned::removeChild(Widget &child)
{
    int index = childIndex(child);
    removeChildInternally(child);

    // Remove the remained child from this paned widget.
    Widget *remained = m_children[1 - index];
    assert(remained);
    removeChildInternally(*remained);

    // Replace this paned widget with the remained child.
    assert(parent());
    parent()->replaceChild(*this, *remained);

    // Destroy this paned widget.
    delete this;
}

void Paned::replaceChild(Widget &oldChild, Widget &newChild)
{
    int index = childIndex(oldChild);
    removeChildInternally(oldChild);
    addChildInternally(newChild, index);
}

Paned *Paned::split(const char *name,
                    Orientation orientation,
                    Widget &child1, Widget &child2)
{
    Widget *original;
    WidgetContainer *parent;
    int newChildIndex;

    if (child1.parent())
    {
        original = &child1;
        newChildIndex = 1;
    }
    else
    {
        assert(child2.parent());
        original = &child2;
        newChildIndex = 0;
    }
    parent = original->parent();

    // Create a paned widget to hold the two widgets.
    Paned *paned = new Paned;
    if (!paned->WidgetContainer::setup(name))
    {
        delete paned;
        return NULL;
    }
    paned->m_orientation = orientation;
    GtkWidget *p = gtk_paned_new(static_cast<GtkOrientation>(orientation));
    g_signal_connect(p, "set-focus-child",
                     G_CALLBACK(setFocusChild), paned);
    paned->setGtkWidget(p);

    // Replace the original widget with the paned widget.
    parent->replaceChild(*original, *paned);

    // Add the two widgets to the paned widget.
    paned->addChildInternally(child1, 0);
    paned->addChildInternally(child2, 1);
    paned->m_currentChildIndex = newChildIndex;

    gtk_widget_show_all(p);
    return paned;
}

void Paned::setFocusChild(GtkWidget *container,
                          GtkWidget *child,
                          Paned *paned)
{
    for (int i = 0; i < paned->childCount(); ++i)
        if (paned->child(i).gtkWidget() == child)
            paned->setCurrentChildIndex(i);
}

}
