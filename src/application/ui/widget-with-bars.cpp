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
                xmlNodeListGetString(doc, child->children, 1);
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

bool WidgetWithBars::XmlElement::restoreWidget(Widget &widget) const
{
    WidgetWithBars &w = static_cast<WidgetWithBars &>(widget);
    w.setCurrentChildIndex(m_currentChildIndex);
}

void WidgetWithBars::XmlElement::removeBar(int index)
{
    delete m_bars[index];
    m_bars.erase(m_bars.begin() + index);
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
    m_verticalGrid = gtk_grid_new();
    m_horizontalGrid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(m_verticalGrid), m_horizontalGrid, 0, 0, 1, 1);
    g_signal_connect(m_verticalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    g_signal_connect(m_horizontalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    addMainChild(mainChild);
}

WidgetWithBars::WidgetWithBars(XmlElement &xmlElement):
    m_mainChild(NULL),
    m_currentChildIndex(0)
{
    Widget *mainChild = xmlElement.mainChild().createWidget();
    if (!mainChild)
        throw std::runtime_error(std::string());
    m_verticalGrid = gtk_grid_new();
    m_horizontalGrid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(m_verticalGrid), m_horizontalGrid, 0, 0, 1, 1);
    g_signal_connect(m_verticalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    g_signal_connect(m_horizontalGrid, "set-focus-child",
                     G_CALLBACK(setFocusChild), this);
    addMainChild(*mainChild);
    for (int i = 0; i < xmlElement.barCount(); ++i)
    {
        Bar *bar = xmlElement.bar(i).createWidget();
        if (!bar)
            xmlElement.removeBar(i);
        else
            addBar(*bar);
    }
}

WidgetWithBars::~WidgetWithBars()
{
    assert(!m_mainChild);
    assert(m_bars.empty());
    gtk_widget_destroy(m_verticalGrid);
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

void WidgetWithBars::onChildClosed(const Widget &child)
{
    if (&child == m_mainChild)
        m_mainChild = NULL;
    else
        m_bars.erase(m_bars.begin() + barIndex(static_cast<Bar &>(child)));
    WidgetContainer.onChildClose(child);
}

void WidgetWithBars::addMainChild(Widget &child)
{
    assert(!child.parent());
    assert(!m_child);
    WidgetContainer::addChild(child);
    m_mainChild = &child;
    child.setParent(this);
    gtk_grid_attach(GTK_GRID(m_horizontalGrid), child.gtkWidget(), 0, 0, 1, 1);
}

void WidgetWithBars::removeMainChild(Widget &child)
{
    assert(&child == m_mainChild);
    assert(child.parent() == this);
    WidgetContainer::removeChild(child);
    m_mainChild = NULL;
    child.setParent(NULL);
    g_object_ref(child.gtkWidget());
    gtk_container_remove(GTK_CONTAINER(m_horizontalGrid), child.gtkWidget());
}

void WidgetWithBars::replaceChild(Widget &oldChild, Widget &newChild)
{
    // Only the main child can be replaced.
    removeMainChild(oldChild);
    addMainChild(newChild);
}

void WidgetWithBars::addBar(Bar &bar)
{
    assert(!bar.parent());
    WidgetContainer::addChild(bar);
    m_bars.push_back(&bar);
    bar.setParent(this);
    if (bar.orientation() == Bar::ORIENTATION_HORIZONTAL)
        gtk_grid_attach_next_to(GTK_GRID(m_verticalGrid),
                                bar.gtkWidget(), NULL,
                                GTK_POS_TOP, 1, 1);
    else
        gtk_grid_attach_next_to(GTK_GRID(m_horizontalGrid),
                                bar.gtkWidget(), NULL,
                                GTK_POS_LEFT, 1, 1);
}

void WidgetWithBars::removeBar(Bar &bar)
{
    assert(bar.parent() == this);
    WidgetContainer::removeChild(bar);
    int index = barIndex(bar);
    m_bars.erase(m_bars.begin() + index);
    bar.setParent(NULL);
    g_object_ref(bar.gtkWidget());
    if (bar.orientation() == Bar::ORIENTATION_HORIZONTAL)
        gtk_container_remove(GTK_CONTAINER(m_verticalGrid), bar.gtkWidget());
    else
        gtk_container_remove(GTK_CONTAINER(m_horizontalGrid), bar.gtkWidget());
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
