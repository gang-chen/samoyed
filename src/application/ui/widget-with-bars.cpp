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
        for (std::vector<Bar::XmlElement *>::size_type i = 0;
             i < m_bars.size();
             ++i)
            delete m_bars[i];
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
    for (std::vector<Bar::XmlElement *>::size_type i = 0;
         i < m_bars.size();
         ++i)
        xmlAddChild(bars, m_bars[i]->write());
    xmlAddChild(node, bars);
    xmlAddChild(node, m_child->write());
}

WidgetWithBars::XmlElement::XmlElement(const WidgetWithBars &widget)
{
    m_child = widget.child().save();
    m_bars.reserve(widget.barCount());

}

}
