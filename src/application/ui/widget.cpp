// Widget.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "widget.hpp"
#include "widget-container.hpp"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <libxml/tree.h>

#define WIDGET "widget"
#define NAME "name"
#define VISIBLE "visible"
#define SAMOYED_WIDGET "samoyed-widget"

namespace Samoyed
{

std::map<std::string, Widget::XmlElement::Reader>
    Widget::XmlElement::s_widgetRegistry;

bool Widget::XmlElement::registerWidget(const char *className,
                                        const Reader &reader)
{
    return s_readerRegistry.insert(std::make_pair(className, reader)).second;
}

Widget::XmlElement* Widget::XmlElement::read(xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &errors)
{
    std::string className(reinterpret_cast<const char *>(node->name));
    std::map<std::string, Reader>::const_iterator it =
        s_readerRegistry.find(className);
    if (it == s_readerRegistry.end())
        // Silently ignore the unknown widget class.
        return NULL;
    return it->second(doc, node, errors);
}

bool Widget::XmlElement::readInternally(xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors)
{
    char *value;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   NAME) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_name = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        VISIBLE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            m_visible = atoi(value);
            xmlFree(value);
        }
    }
    return true;
}

xmlNodePtr Widget::XmlElement::write() const
{
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(WIDGET));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(NAME),
                    reinterpret_cast<const xmlChar *>(m_name.c_str()));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(VISIBLE),
                    reinterpret_cast<const xmlChar *>(m_visible ? "1" : "0"));
    return node;
}

void Widget::XmlElement::saveWidgetInternally(const Widget &widget)
{
    m_name = widget.name();
    m_visible = gtk_widget_get_visible(widget.gtkWidget());
}

bool Widget::setup(const char *name)
{
    m_name = name;
    return true;
}

bool Widget::restore(XmlElement &xmlElement)
{
    m_name = xmlElement.name();
    return true;
}

Widget::~Widget()
{
    if (m_parent)
        m_parent->removeChild(*this);
    if (m_gtkWidget)
        gtk_widget_destroy(m_gtkWidget);
}

void Widget::setTitle(const char *title)
{
    m_title = title;
    if (m_parent)
        m_parent->onChildTitleChanged(*this);
}

void Widget::setDescription(const char *description)
{
    m_description = description;
    if (m_parent)
        m_parent->onChildDescriptionChanged(*this);
}

void Widget::setCurrent()
{
    // The purpose is to switch the current page in each notebook to the desired
    // one.
    Widget *child = this;
    WidgetContainer *parent = child->parent();
    while (parent)
    {
        parent->setCurrentChildIndex(parent->childIndex(*child));
        child = parent;
        parent = parent->parent();
    }

    // Set this widget as the current widget.
    gtk_widget_grab_focus(gtkWidget());
    gtk_window_present(GTK_WINDOW(child->gtkWidget()));
}

void Widget::setGtkWidget(GtkWidget *gtkWidget)
{
    // Cannot change the GTK+ widget.
    assert(!m_gtkWidget);
    m_gtkWidget = gtkWidget;
    g_object_set_data(G_OBJECT(gtkWidget), SAMOYED_WIDGET, this);
}

Widget *Widget::getFromGtkWidget(GtkWidget *gtkWidget)
{
    return static_cast<Widget *>(g_object_get_data(G_OBJECT(gtkWidget),
                                                   SAMOYED_WIDGET));
}

}
