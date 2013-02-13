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
    m_child(NULL)
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
        else
        {
            Widget::XmlElement *ch
                = Widget::XmlElement::read(doc, child, errors);
            if (ch)
            {
                if (m_child)
                {
                    cp = g_strdup_printf(
                        _("Line %d: More than one children contained by the "
                          "bin.\n"),
                        child->line);
                    errors.push_back(std::string(cp));
                    g_free(cp);
                    delete ch;
                }
                else
                    m_child = ch;
            }
        }
    }

    if (!m_child)
    {
        cp = g_strdup_printf(
            _("Line %d: No child contained by the bin.\n"),
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
    xmlNodePtr bars = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>("bars"));
    for (std::vector<Bar::XmlElement *>::const_iterator it = m_bars.begin();
         it != m_bars.end();
         ++it)
        xmlAddChild(bars, (*it)->write());
    xmlAddChild(node, bars);
    xmlAddChild(node, m_child->write());
}

WidgetWithBars::XmlElement::XmlElement(const WidgetWithBars &widget)
{
    m_child = widget.child().save();
    m_bars.reserve(widget.m_barTable.size());
    for (BarTable::const_iterator it = widget.m_barTable.begin();
         it != widget.m_barTable.end();
         ++it)
        m_bars.push_back(it->second->save());
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
}

void WidgetWithBars::XmlElement::removeBar(int index)
{
    delete m_bars[index];
    m_bars.erase(m_bars.begin() + index);
}

WidgetWithBars::XmlElement::~XmlElement()
{
    delete m_child;
    for (std::vector<Bar::XmlElement *>::const_iterator it = m_bars.begin();
         it != m_bars.end();
         ++it)
        delete *it;
}

WidgetWithBars::WidgetWithBars():
    m_child(NULL)
{
    m_verticalGrid = gtk_grid_new();
    m_horizontalGrid = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(m_verticalGrid),
                            m_horizontalGrid, NULL,
                            GTK_POS_BOTTOM, 1, 1);
}

WidgetWithBars::WidgetWithBars(XmlElement &xmlElement):
    m_child(NULL)
{
    Widget *child = xmlElement.child().createWidget();
    if (!child)
        throw std::runtime_error(std::string());
    m_verticalGrid = gtk_grid_new();
    m_horizontalGrid = gtk_grid_new();
    gtk_grid_attach_next_to(GTK_GRID(m_verticalGrid),
                            m_horizontalGrid, NULL,
                            GTK_POS_BOTTOM, 1, 1);
    addChild(*child);
    for (int i = 0; i < xmlElement.barCount(); ++i)
    {
        Bar *bar = xmlElement.child(i).createWidget();
        if (!bar)
            xmlElement.removeBar(i);
        else if (findBar(*bar))
        {
            xmlElement.removeBar(i);
            delete bar;
        }
        else
            addBar(*bar);
    }
}

WidgetWithBars::~WidgetWithBars()
{
    assert(!m_child);
    assert(m_barTable.empty());
    gtk_widget_destroy(m_verticalGrid);
}

bool WidgetWithBars::close()
{
    if (closing())
        return true;

    setClosing(true);

    std::vector<Bar *> bars;
    bars.reserve(m_barTable.size());
    for (BarTable::const_iterator it = m_barTable.begin();
         it != m_barTable.end();
         ++it)
        bars.push_back(it->second);
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
    if (!m_child->close())
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

void WidgetWithBars::onChildClosed(Widget *child)
{
    assert(m_child == child);
    assert(closing());
    m_child = NULL;
    if (m_barTable.empty())
        delete this;
}

void WidgetWithBars::addChild(Widget &child)
{
    assert(!child.parent());
    assert(!m_child);
    m_child = &child;
    child.setParent(this);
    gtk_grid_attach_next_to(GTK_GRID(m_horizontalGrid),
                            child.gtkWidget(), NULL,
                            GTK_POS_RIGHT, 1, 1);
}

void WidgetWithBars::removeChild(Widget &child)
{
    assert(child.parent() == this);
    m_child = NULL;
    child.setParent(NULL);
    gtk_container_remove(GTK_CONTAINER(m_horizontalGrid), child.gtkWidget());
}

void WidgetWithBars::replaceChild(Widget &oldChild, Widget &newChild)
{
    removeChild(oldChild);
    addChild(newChild);
}

void WidgetWithBars::onBarClosed(const Bar *bar)
{
}

void WidgetWithBars::addBar(Bar &bar)
{
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
}

}
