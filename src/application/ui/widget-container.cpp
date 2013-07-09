// Widget container.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "widget-container.hpp"
#include <assert.h>
#include <string.h>
#include <list>
#include <string>
#include <utility>
#include <glib.h>
#include <glib/gi18n.h>
#include <libxml/tree.h>

#define WIDGET "widget"
#define WIDGET_CONTAINER "widget-container"

namespace Samoyed
{

bool
WidgetContainer::XmlElement::readInternally(xmlNodePtr node,
                                            std::list<std::string> &errors)
{
    char *cp;
    bool widgetSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
        if (child->type != XML_ELEMENT_NODE)
            continue;
        if (strcmp(reinterpret_cast<const char *>(child->name),
                   WIDGET) == 0)
        {
            if (widgetSeen)
            {
                cp = g_strdup_printf(
                    _("Line %d: More than one \"%s\" elements seen.\n"),
                    child->line, WIDGET);
                errors.push_back(cp);
                g_free(cp);
                return false;
            }
            if (!Widget::XmlElement::readInternally(child, errors))
                return false;
            widgetSeen = true;
        }
    }
    if (!widgetSeen)
    {
        cp = g_strdup_printf(
            _("Line %d: \"%s\" element missing.\n"),
            node->line, WIDGET);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    return true;
}

xmlNodePtr WidgetContainer::XmlElement::write() const
{
    xmlNodePtr node =
        xmlNewNode(NULL,
                   reinterpret_cast<const xmlChar *>(WIDGET_CONTAINER));
    xmlAddChild(node, Widget::XmlElement::write());
    return node;
}

void WidgetContainer::grabFocus()
{
    Widget &current = this->current();
    if (&current == this)
        Widget::grabFocus();
    else
        current.grabFocus();
}

Widget &WidgetContainer::current()
{
    return (currentChildIndex() >= 0 ?
            child(currentChildIndex()).current() : *this);
}

const Widget &WidgetContainer::current() const
{
    return (currentChildIndex() >= 0 ?
            child(currentChildIndex()).current() : *this);
}

int WidgetContainer::childIndex(const Widget &child) const
{
    for (int i = 0; i < childCount(); ++i)
        if (&this->child(i) == &child)
            return i;
    return -1;
}

Widget *WidgetContainer::findChild(const char *id)
{
    WidgetTable::const_iterator it = m_childTable.find(id);
    if (it == m_childTable.end())
        return NULL;
    return it->second;
}

const Widget *WidgetContainer::findChild(const char *id) const
{
    WidgetTable::const_iterator it = m_childTable.find(id);
    if (it == m_childTable.end())
        return NULL;
    return it->second;
}

void WidgetContainer::addChildInternally(Widget &child)
{
    assert(!child.parent());
    child.setParent(this);
    m_childTable.insert(std::make_pair(child.id(), &child));
}

void WidgetContainer::removeChildInternally(Widget &child)
{
    assert(child.parent() == this);
    child.setParent(NULL);
    m_childTable.erase(child.id());
    m_childRemoved(*this, child);
}

void WidgetContainer::destroyChild(Widget &child)
{
    // The removal of the child widget may cause this widget container to be
    // destroyed.  Postpone destroying this widget container until the child is
    // destroyed.
    m_postponeDestroy = true;
    removeChild(child);
    m_postponeDestroy = false;
    delete &child;

    // Destroy this widget container.
    if (m_requestDestroy)
        destroy();
}

void WidgetContainer::destroyInternally()
{
    if (m_postponeDestroy)
        m_requestDestroy = true;
    else
        destroy();
}

}
