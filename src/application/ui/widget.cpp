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
#include <glib/gi18n.h>
#include <libxml/tree.h>

#define WIDGET "widget"
#define ID "id"
#define TITLE "title"
#define DESCRIPTION "description"
#define VISIBLE "visible"
#define PROPERTIES "properties"
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

Widget::XmlElement* Widget::XmlElement::read(xmlNodePtr node,
                                             std::list<std::string> *errors)
{
    std::string className(reinterpret_cast<const char *>(node->name));
    std::map<std::string, Reader>::const_iterator it =
        s_readerRegistry.find(className);
    if (it == s_readerRegistry.end())
        // Silently ignore the unknown widget class.
        return NULL;
    return it->second(node, errors);
}

bool Widget::XmlElement::readInternally(xmlNodePtr node,
                                        std::list<std::string> *errors)
{
    char *value, *cp;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   ID) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_id = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        TITLE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_title = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        DESCRIPTION) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_description = value;
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        VISIBLE) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                try
                {
                    m_visible = boost::lexical_cast<bool>(value);
                }
                catch (boost::bad_lexical_cast &error)
                {
                    if (errors)
                    {
                        cp = g_strdup_printf(
                            _("Line %d: Invalid Boolean value \"%s\" for "
                              "element \"%s\". %s.\n"),
                            child->line, value, VISIBLE, error.what());
                        errors->push_back(cp);
                        g_free(cp);
                    }
                }
                xmlFree(value);
            }
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        PROPERTIES) == 0)
        {
            for (xmlNodePtr grandChild = child->children;
                 grandChild;
                 grandChild = grandChild->next)
            {
                if (grandChild->type != XML_ELEMENT_NODE)
                    continue;
                value = reinterpret_cast<char *>(
                    xmlNodeGetContent(grandChild->children));
                if (value)
                {
                    m_properties.insert(std::make_pair(
                        reinterpret_cast<const char *>(grandChild->name),
                        value));
                    xmlFree(value);
                }
            }
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
    xmlNodePtr prop = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(PROPERTIES));
    for (PropertyMap::const_iterator it = m_properties.begin();
         it != m_properties.end();
         ++it)
    {
        xmlNewTextChild(prop, NULL,
                        reinterpret_cast<const xmlChar *>(it->first.c_str()),
                        reinterpret_cast<const xmlChar *>(it->second.c_str()));
    }
    xmlAddChild(node, prop);
    return node;
}

Widget::XmlElement::XmlElement(const Widget &widget):
    m_id(widget.id()),
    m_title(widget.title()),
    m_description(widget.description()),
    m_visible(gtk_widget_get_visible(widget.gtkWidget())),
    m_properties(widget.properties())
{
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
    m_properties = xmlElement.properties();
    return true;
}

Widget::~Widget()
{
    assert(!m_parent);
    m_closed(*this);
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

const std::string *Widget::getProperty(const char *name) const
{
    PropertyMap::const_iterator it = m_properties.find(name);
    if (it == m_properties.end())
        return NULL;
    return &it->second;
}

void Widget::setProperty(const char *name, const std::string &value)
{
    m_properties[name] = value;
}

void Widget::destroy()
{
    if (parent())
        parent()->destroyChild(*this);
    else
        delete this;
}

bool Widget::close()
{
    if (closing())
        return true;
    setClosing(true);
    destroy();
    return true;
}

}
