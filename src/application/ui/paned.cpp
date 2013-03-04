// Paned widget.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "paned.hpp"
#include <assert.h>
#include <stdlib.h>
#include <list>
#include <stdexcept>
#include <string>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

bool Paned::XmlElement::registerReader()
{
    return Widget::XmlElement::registerReader("paned",
                                              Widget::XmlElement::Reader(read));
}

Paned::XmlElement::XmlElement(xmlDocPtr doc,
                              xmlNodePtr node,
                              std::list<std::string> &errors):
    m_orientation(Paned::ORIENTATION_HORIZONTAL),
    m_currentChildIndex(0),
    m_position(-1)
{
    m_children[0] = NULL;
    m_children[1] = NULL;

    char *value, *cp;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   "orientation") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (strcmp(value, "horizontal") == 0)
                m_orientation = Paned::ORIENTATION_HORIZONTAL;
            else if (strcmp(value, "vertical") == 0)
                m_orientation = Paned::ORIENTATOIN_VERTICAL;
            else
            {
                cp = g_strdup_printf(
                    _("Line %d: Unknown orientation \"%s\".\n"),
                    child->line, value);
                errors.push_back(std::string(cp));
                g_free(cp);
            }
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
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        "position") == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1);
            m_position = atoi(value);
            xmlFree(value);
        }
        else
        {
            Widget::XmlElement *ch =
                Widget::XmlElement::read(doc, child, errors);
            if (ch)
            {
                if (m_children[1])
                {
                    cp = g_strdup_printf(
                        _("Line %d: More than two children contained by the "
                          "paned widget.\n"),
                        child->line);
                    errors.push_back(std::string(cp));
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

    if (!m_children[0])
    {
        cp = g_strdup_printf(
            _("Line %d: No child contained by the paned widget.\n"),
            node->line);
        errors.push_back(std::string(cp));
        g_free(cp);
        throw std::runtime_error(std::string());
    }
    if (!m_children[1])
    {
        cp = g_strdup_printf(
            _("Line %d: Only one child contained by the paned widget.\n"),
            node->line);
        errors.push_back(std::string(cp));
        g_free(cp);
        delete m_children[0];
        throw std::runtime_error(std::string());
    }

    if (m_currentChildIndex < 0)
        m_currentChildIndex = 0;
    else if (m_currentChildIndex > 1)
        m_currentChildIndex = 1;
}

Widget::XmlElement *Paned::XmlElement::read(xmlDocPtr doc,
                                            xmlNodePtr node,
                                            std::list<std::string> &errors)
{
    XmlElement *element;
    try
    {
        element = new XmlElement(doc, node, errors);
    }
    catch (const std::runtime_error &exception)
    {
        return NULL;
    }
    return element;
}

xmlNodePtr Paned::XmlElement::write() const
{
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>("paned"));
    char *cp;
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>("orientation"),
                    m_orientation ==
                    Samoyed::SplitPane::ORIENTATION_HORIZONTAL ?
                    reinterpret_cast<const xmlChar *>("horizontal") :
                    reinterpret_cast<const xmlChar *>("vertical"));
    cp = g_strdup_printf("%d", m_currentChildIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>("current-child-index"),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    cp = g_strdup_printf("%d", m_position);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>("position"),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    xmlAddChild(node, m_children[0]->write());
    xmlAddChild(node, m_children[1]->write());
    return node;
}

Paned::XmlElement::XmlElement(const Paned &paned)
{
    m_orientation = paned.orientation();
    m_children[0] = paned.child(0).save();
    m_children[1] = paned.child(1).save();
    m_currentChildIndex = paned.currentChildIndex();
    m_position = paned.position();
}

Widget *Paned::XmlElement::createWidget()
{
    Paned *paned;
    try
    {
        paned = new Paned(*this);
    }
    catch (const std::runtime_error &exception)
    {
        return NULL;
    }
    return paned;
}

Paned::XmlElement::~XmlElement()
{
    delete m_children[0];
    delete m_children[1];
}

Paned::Paned(const char *name, Orientation orientation):
    WidgetContainer(name),
    m_orientation(orientation),
    m_currentChildIndex(0)
{
    m_children[0] = NULL;
    m_children[1] = NULL;
    GtkWidget *paned = gtk_paned_new(static_cast<GtkOrientation>(orientation));
    g_signal_connect(paned, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(paned);
}

Paned::Paned(const char *name,
             Orientation orientation,
             Widget &child1, Widget &child2):
    WidgetContainer(name),
    m_orientation(orientation),
    m_currentChildIndex(0)
{
    m_children[0] = NULL;
    m_children[1] = NULL;
    GtkWidget *paned = gtk_paned_new(static_cast<GtkOrientation>(orientation));
    g_signal_connect(paned, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(paned);
    addChildInternally(child1, 0);
    addChildInternally(child2, 1);
}

Paned::Paned(const XmlElement &xmlElement):
    WidgetContainer(xmlElement)
{
    m_children[0] = NULL;
    m_children[1] = NULL;
    Widget *child1 = xmlElement.child(0).createWidget();
    if (!child1)
        throw std::runtime_error(std::string());
    Widget *child2 = xmlElement.child(1).createWidget();
    if (!child2)
    {
        delete child1;
        throw std::runtime_error(std::string());
    }
    GtkWidget *paned =
        gtk_paned_new(static_cast<GtkOrientation>(xmlElement.m_orientation));
    g_signal_connect(paned, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(paned);
    addChildInternally(*child1, 0);
    addChildInternally(*child2, 1);
    m_currentChildIndex = xmlElement.currentChildIndex();
    if (xmlElement.m_position >= 0)
        setPosition(xmlElement.position());
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
        gtk_paned_add1(GTK_PANED(gtkWidget()), child.gtkWidget());
    else
        gtk_paned_add2(GTK_PANED(gtkWidget()), child.gtkWidget());
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
    int index;

    if (child1.parent())
        original = &child1;
    else
    {
        assert(child2.parent());
        original = &child2;
    }
    parent = original->parent();
    index = parent->childIndex(*original);

    // Create a paned widget to hold the two widgets.
    Paned *paned = new Paned(name, orientation);

    // Replace the original widget with the paned widget.
    parent->replaceChild(*original, *paned);

    // Add the two widgets to the paned widget.
    paned->addChildInternally(child1, 0);
    paned->addChildInternally(child2, 1);

    return paned;
}

void Paned::setFocusChild(GtkWidget *container,
                          GtkWidget *child,
                          gpointer paned)
{
    Paned *p = static_cast<Paned *>(paned);
    if (p->m_children[0]->gtkWidget() == child)
        m_currentChildIndex = 0;
    else
    {
        assert(p->m_children[1]->gtkWidget() == child);
        m_currentChildIndex = 1;
    }
}

}
