// Widget.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "widget.hpp"
#include "widget-container.hpp"
#include <assert.h>
#include <string.h>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <boost/lexical_cast.hpp>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/tree.h>

#define WIDGET "widget"
#define ID "id"
#define TITLE "title"
#define DESCRIPTION "description"
#define VISIBLE "visible"
#define SAMOYED_WIDGET "samoyed-widget"

namespace Samoyed
{

std::map<std::string, Widget::XmlElement::Reader>
    Widget::XmlElement::s_readerRegistry;

void Widget::XmlElement::registerReader(const char *className,
                                        const Reader &reader)
{
    s_readerRegistry.insert(std::make_pair(className, reader));
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
    char *value, *cp;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   ID) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                m_id = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        TITLE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                m_title = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        DESCRIPTION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            if (value)
                m_description = value;
            xmlFree(value);
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        VISIBLE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeListGetString(doc, child->children, 1));
            try
            {
                m_visible = boost::lexical_cast<bool>(value);
            }
            catch (boost::bad_lexical_cast &exp)
            {
                cp = g_strdup_printf(
                    _("Line %d: Invalid Boolean value \"%s\" for element "
                      "\"%s\". %s.\n"),
                    child->line, value, VISIBLE, exp.what());
                errors.push_back(cp);
                g_free(cp);
            }
            xmlFree(value);
        }
    }
    return true;
}

xmlNodePtr Widget::XmlElement::write() const
{
    std::string str;
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(WIDGET));
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(ID),
                    reinterpret_cast<const xmlChar *>(id()));
    if (!m_title.empty())
        xmlNewTextChild(node, NULL,
                        reinterpret_cast<const xmlChar *>(TITLE),
                        reinterpret_cast<const xmlChar *>(title()));
    if (!m_description.empty())
        xmlNewTextChild(node, NULL,
                        reinterpret_cast<const xmlChar *>(DESCRIPTION),
                        reinterpret_cast<const xmlChar *>(description()));
    str = boost::lexical_cast<std::string>(m_visible);
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(VISIBLE),
                    reinterpret_cast<const xmlChar *>(str.c_str()));
    return node;
}

Widget::XmlElement::XmlElement(const Widget &widget)
{
    m_id = widget.id();
    m_title = widget.title();
    m_description = widget.description();
    m_visible = gtk_widget_get_visible(widget.gtkWidget());
}

bool Widget::setup(const char *id)
{
    m_id = id;
    return true;
}

bool Widget::restore(XmlElement &xmlElement)
{
    m_id = xmlElement.id();
    m_title = xmlElement.title();
    m_description = xmlElement.description();
    return true;
}

Widget::~Widget()
{
    m_closed(*this);
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
    grabFocus();
    if (!gtk_widget_get_visible(child->gtkWidget()))
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
