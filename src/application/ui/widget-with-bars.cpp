// Widget with bars.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "widget-with-bars.hpp"
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

#define WIDGET_CONTAINER "widget-container"
#define WIDGET_WITH_BARS "widget-with-bars"
#define MAIN_CHILD "main-child"
#define BARS "bars"
#define CURRENT_CHILD_INDEX "current-child-index"

namespace Samoyed
{

bool WidgetWithBars::XmlElement::registerReader()
{
    return Widget::XmlElement::registerReader(WIDGET_WITH_BARS,
                                              Widget::XmlElement::Reader(read));
}

bool WidgetWithBars::XmlElement::readInternally(xmlDocPtr doc,
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
                        MAIN_CHILD) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                Widget::XmlElement *ch =
                    Widget::XmlElement::read(doc, grandChild, errors);
                if (ch)
                {
                    if (m_mainChild)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: More than one main children contained "
                              "by the bin.\n"),
                            grandChild->line);
                        errors.push_back(cp);
                        g_free(cp);
                        delete ch;
                    }
                    else
                        m_mainChild = ch;
                }
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        BARS) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                Bar::XmlElement *bar =
                    Bar::XmlElement::read(doc, grandChild, errors);
                if (bar)
                    m_bars.push_back(bar);
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

    if (!containerSeen)
    {
        cp = g_strdup_printf(
            _("Line %d: \"%s\" element missing.\n"),
            node->line, WIDGET_CONTAINER);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }

    if (!m_mainChild)
    {
        cp = g_strdup_printf(
            _("Line %d: No main child contained by the bin.\n"),
            node->line);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }

    if (m_currentChildIndex < 0)
        m_currentChildIndex = 0;
    else if (m_currentChildIndex == barCount() + 1)
        m_currentChildIndex = barCount();
    return true;
}

WidgetWithBars::XmlElement *
WidgetWithBars::XmlElement::read(xmlDocPtr doc,
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

xmlNodePtr WidgetWithBars::XmlElement::write() const
{
    char *cp;
    xmlNodePtr node =
        xmlNewNode(NULL, reinterpret_cast<const xmlChar *>(WIDGET_WITH_BARS));
    xmlAddChild(node, WidgetContainer::XmlElement::write());
    xmlNodePtr mainChild =
        xmlNewNode(NULL, reinterpret_cast<const xmlChar *>(MAIN_CHILD));
    xmlAddChild(mainChild, m_mainChild->write());
    xmlAddChild(node, mainChild);
    xmlNodePtr bars =
        xmlNewNode(NULL, reinterpret_cast<const xmlChar *>(BARS));
    for (std::vector<Bar::XmlElement *>::const_iterator it = m_bars.begin();
         it != m_bars.end();
         ++it)
        xmlAddChild(bars, (*it)->write());
    xmlAddChild(node, bars);
    cp = g_strdup_printf("%d", m_currentChildIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(CURRENT_CHILD_INDEX),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    return node;
}

WidgetWithBars::XmlElement::XmlElement(const WidgetWithBars &widget):
    WidgetContainer::XmlElement(widget)
{
    m_mainChild = widget.mainChild().save();
    m_bars.reserve(widget.barCount());
    m_currentChildIndex = widget.currentChildIndex();
    for (int i = 0; i < widget.barCount(); ++i)
    {
        Bar::XmlElement *bar = widget.bar(i).save();
        if (bar)
            m_bars.push_back(bar);
        else
        {
            // Some bars are not restorable.  Remove them.
            if (m_currentChildIndex > i + 1)
                --m_currentChildIndex;
        }
    }
    if (m_currentChildIndex == barCount() + 1)
        m_currentChildIndex = barCount();
}

Widget *WidgetWithBars::XmlElement::restoreWidget()
{
    WidgetWithBars *widget = new WidgetWithBars;
    if (!widget->restore(*this))
    {
        delete widget;
        return NULL;
    }
    return widget;
}

void WidgetWithBars::XmlElement::removeBar(int index)
{
    delete m_bars[index];
    m_bars.erase(m_bars.begin() + index);
    if (m_currentChildIndex > index + 1)
        --m_currentChildIndex;
    else if (m_currentChildIndex == barCount() + 1)
        m_currentChildIndex = barCount();
}

WidgetWithBars::XmlElement::~XmlElement()
{
    delete m_mainChild;
    for (std::vector<Bar::XmlElement *>::const_iterator it = m_bars.begin();
         it != m_bars.end();
         ++it)
        delete *it;
}

bool WidgetWithBars::setup(const char *id, Widget &mainChild)
{
    if (!WidgetContainer::setup(id))
        return false;
    GtkWidget *verticalGrid = gtk_grid_new();
    m_horizontalGrid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(verticalGrid), m_horizontalGrid, 0, 0, 1, 1);
    g_signal_connect(verticalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    g_signal_connect(m_horizontalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(verticalGrid);
    gtk_widget_show_all(verticalGrid);
    addMainChild(mainChild);
    return true;
}

WidgetWithBars *WidgetWithBars::create(const char *id, Widget &mainChild)
{
    WidgetWithBars *widget = new WidgetWithBars;
    if (!widget->setup(id, mainChild))
    {
        delete widget;
        return NULL;
    }
    return widget;
}

bool WidgetWithBars::restore(XmlElement &xmlElement)
{
    if (!WidgetContainer::restore(xmlElement))
        return false;
    Widget *mainChild = xmlElement.mainChild().restoreWidget();
    if (!mainChild)
        return false;
    GtkWidget *verticalGrid = gtk_grid_new();
    m_horizontalGrid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(verticalGrid), m_horizontalGrid, 0, 0, 1, 1);
    g_signal_connect(verticalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    g_signal_connect(m_horizontalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(verticalGrid);
    if (xmlElement.visible())
        gtk_widget_show_all(verticalGrid);
    addMainChild(*mainChild);
    m_bars.reserve(xmlElement.barCount());
    for (int i = 0; i < xmlElement.barCount(); ++i)
    {
        Bar *bar = static_cast<Bar *>(xmlElement.bar(i).restoreWidget());
        if (!bar)
        {
            xmlElement.removeBar(i);
            --i;
        }
        else
            addBar(*bar);
    }
    setCurrentChildIndex(xmlElement.currentChildIndex());
    return true;
}

WidgetWithBars::~WidgetWithBars()
{
    assert(!m_mainChild);
    assert(m_bars.empty());
}

bool WidgetWithBars::close()
{
    if (closing())
        return true;

    setClosing(true);

    std::vector<Bar *> bars(m_bars);
    for (std::vector<Bar *>::const_iterator it = bars.begin();
         it != bars.end();
         ++it)
    {
        if (!(*it)->close())
        {
            setClosing(false);
            return false;
        }
    }
    if (!m_mainChild->close())
    {
        setClosing(false);
        return false;
    }
    return true;
}

Widget::XmlElement *WidgetWithBars::save() const
{
    return new XmlElement(*this);
}

void WidgetWithBars::addMainChild(Widget &child)
{
    assert(!m_mainChild);
    WidgetContainer::addChildInternally(child);
    m_mainChild = &child;
    gtk_grid_attach(GTK_GRID(m_horizontalGrid), child.gtkWidget(), 0, 0, 1, 1);
}

void WidgetWithBars::removeMainChild(Widget &child)
{
    assert(&child == m_mainChild);
    m_mainChild = NULL;
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTAINER(m_horizontalGrid), child.gtkWidget());
    WidgetContainer::removeChildInternally(child);
}

void WidgetWithBars::addBar(Bar &bar)
{
    WidgetContainer::addChildInternally(bar);
    m_bars.push_back(&bar);
    if (bar.orientation() == Bar::ORIENTATION_HORIZONTAL)
        gtk_grid_attach_next_to(GTK_GRID(gtkWidget()),
                                bar.gtkWidget(), NULL,
                                GTK_POS_TOP, 1, 1);
    else
        gtk_grid_attach_next_to(GTK_GRID(m_horizontalGrid),
                                bar.gtkWidget(), NULL,
                                GTK_POS_LEFT, 1, 1);
}

void WidgetWithBars::removeBar(Bar &bar)
{
    int index = barIndex(bar);
    m_bars.erase(m_bars.begin() + index);
    g_object_ref(bar.gtkWidget());
    if (bar.orientation() == Bar::ORIENTATION_HORIZONTAL)
        gtk_container_remove(GTK_CONTAINER(gtkWidget()), bar.gtkWidget());
    else
        gtk_container_remove(GTK_CONTAINER(m_horizontalGrid), bar.gtkWidget());
    WidgetContainer::removeChildInternally(bar);
}

void WidgetWithBars::removeChild(Widget &child)
{
    // Only the main child can be removed.  And all the child bars are already
    // removed.
    assert(closing());
    assert(barCount() == 0);
    removeMainChild(child);
    delete this;
}

void WidgetWithBars::replaceChild(Widget &oldChild, Widget &newChild)
{
    // Only the main child can be replaced.
    removeMainChild(oldChild);
    addMainChild(newChild);
}

int WidgetWithBars::barIndex(const Bar &bar) const
{
    for (int i = 0; i < barCount(); ++i)
        if (&this->bar(i) == &bar)
            return i;
    return -1;
}

void WidgetWithBars::setFocusChild(GtkWidget *container,
                                   GtkWidget *child,
                                   WidgetWithBars *widget)
{
    if (widget->m_mainChild && widget->m_mainChild->gtkWidget() == child)
        widget->setCurrentChildIndex(0);
    for (int i = 0; i < widget->barCount(); ++i)
        if (widget->m_bars[i] && widget->m_bars[i]->gtkWidget() == child)
            widget->setCurrentChildIndex(i + 1);
}

}
