// Widget with bars.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "widget-with-bars.hpp"
#include <assert.h>
#include <stdlib.h>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

bool WidgetWithBars::XmlElement::registerReader()
{
    return Widget::XmlElement::registerReader("widget-with-bars",
                                              Widget::XmlElement::Reader(read));
}

WidgetWithBars::XmlElement::XmlElement(xmlDocPtr doc,
                                       xmlNodePtr node,
                                       std::list<std::string> &errors):
    m_mainChild(NULL),
    m_currentChildIndex(0)
{
    char *value, *cp;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name), "bars") == 0)
        {
            for (xmlNodePtr bar = child->children; bar; bar = bar->next)
            {
                Bar::XmlElement *b = Bar::XmlElement::read(doc, bar, errors);
                if (b)
                    m_bars.push_back(b);
            }
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
            Widget::XmlElement *ch
                = Widget::XmlElement::read(doc, child, errors);
            if (ch)
            {
                if (m_mainChild)
                {
                    cp = g_strdup_printf(
                        _("Line %d: More than one main children contained by "
                          "the bin.\n"),
                        child->line);
                    errors.push_back(std::string(cp));
                    g_free(cp);
                    delete ch;
                }
                else
                    m_mainChild = ch;
            }
        }
    }

    if (!m_mainChild)
    {
        cp = g_strdup_printf(
            _("Line %d: No main child contained by the bin.\n"),
            node->line);
        errors.push_back(std::string(cp));
        g_free(cp);
        for (std::vector<Bar::XmlElement *>::const_iterator it = m_bars.begin();
             it != m_bars.end();
             ++it)
            delete *it;
        throw std::runtime_error(std::string());
    }

    if (m_currentChildIndex < 0)
        m_currentChildIndex = 0;
    else if (m_currentChildIndex == barCount() + 1)
        m_currentChildIndex = barCount();
}

Widget::XmlElement *
WidgetWithBars::XmlElement::read(xmlDocPtr doc,
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

xmlNodePtr WidgetWithBars::XmlElement::write() const
{
    xmlNodePtr node =
        xmlNewNode(NULL, reinterpret_cast<const xmlChar *>("widget-with-bars"));
    char *cp;
    cp = g_strdup_printf("%d", m_currentChildIndex);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>("current-child-index"),
                    reinterpret_cast<const xmlChar *>(cp));
    g_free(cp);
    xmlNodePtr bars = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>("bars"));
    for (std::vector<Bar::XmlElement *>::const_iterator it = m_bars.begin();
         it != m_bars.end();
         ++it)
        xmlAddChild(bars, (*it)->write());
    xmlAddChild(node, bars);
    xmlAddChild(node, m_mainChild->write());
}

WidgetWithBars::XmlElement::XmlElement(const WidgetWithBars &widget)
{
    m_mainChild = widget.mainChild().save();
    m_bars.reserve(widget.barCount());
    for (int i = 0; i < widget.barCount(); ++i)
        m_bars.push_back(widget.bar(i).save());
    m_currentChildIndex = widget.currentChildIndex();
}

Widget *WidgetWithBars::XmlElement::createWidget()
{
    WidgetWithBars *widget;
    try
    {
        widget = new WidgetWithBars(*this);
    }
    catch (const std::runtime_error &exception)
    {
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
        m_currentChildIndex = barCount() - 1;
}

WidgetWithBars::XmlElement::~XmlElement()
{
    delete m_mainChild;
    for (std::vector<Bar::XmlElement *>::const_iterator it = m_bars.begin();
         it != m_bars.end();
         ++it)
        delete *it;
}

WidgetWithBars::WidgetWithBars(const char *name, Widget &mainChild):
    WidgetContainer(name),
    m_mainChild(NULL),
    m_currentChildIndex(0)
{
    GtkWidget *verticalGrid = gtk_grid_new();
    m_horizontalGrid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(verticalGrid), m_horizontalGrid, 0, 0, 1, 1);
    g_signal_connect(verticalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    g_signal_connect(m_horizontalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(verticalGrid);
    addMainChild(mainChild);
}

WidgetWithBars::WidgetWithBars(XmlElement &xmlElement):
    WidgetContainer(xmlElement),
    m_mainChild(NULL)
{
    Widget *mainChild = xmlElement.mainChild().createWidget();
    if (!mainChild)
        throw std::runtime_error(std::string());
    GtkWidget *verticalGrid = gtk_grid_new();
    m_horizontalGrid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(verticalGrid), m_horizontalGrid, 0, 0, 1, 1);
    g_signal_connect(verticalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    g_signal_connect(m_horizontalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    setGtkWidget(verticalGrid);
    addMainChild(*mainChild);
    m_bars.reserve(xmlElement.barCount());
    for (int i = 0; i < xmlElement.barCount(); ++i)
    {
        Bar *bar = xmlElement.bar(i).createWidget();
        if (!bar)
            xmlElement.removeBar(i);
        else
            addBar(*bar);
    }
    setCurrentChildIndex(xmlElement.currentChildIndex());
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

    std::vector<Bar *> bars(bars);
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
    assert(!m_child);
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

void WidgetWithBars::addBarInternally(Bar &bar)
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

void WidgetWithBars::removeBarInternally(Bar &bar)
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

void WidgetWithBars::removeChildInternally(Widget &child)
{
    if (&child == m_mainChild)
        removeMainChild(child);
    else
        removeBarInternally(static_cast<Bar &>(child));
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
                                   gpointer widget)
{
    WidgetWithBars *w = static_cast<WidgetWithBars *>(widget);
    if (w->m_mainChild->gtkWidget() == child)
        setCurrentChildIndex(0);
    else
    {
        for (std::vector<Bar *>::size_type i = 0; i < m_bars.size(); ++i)
            if (m_bars[i]->gtkWidget() == child)
                setCurrentChildIndex(i);
    }
}

}
