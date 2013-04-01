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
#include <glib/gi18n-lib.h>
#include <libxml/tree.h>

#define WIDGET "widget"
#define WIDGET_CONTAINER "widget-container"

namespace Samoyed
{

bool
WidgetContainer::XmlElement::readInternally(xmlDocPtr doc,
                                            xmlNodePtr node,
                                            std::list<std::string> &errors)
{
    char *cp;
    bool widgetSeen = false;
    for (xmlNodePtr child = node->children; child; child = child->next)
    {
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
            if (!Widget::XmlElement::readInternally(doc, child, errors))
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

void WidgetContainer::removeChild(Widget &child)
{
    removeChildInternally(child);
    if (closing() && childCount() == 0)
        delete this;
}

int WidgetContainer::childIndex(const Widget &child) const
{
    for (int i = 0; i < childCount(); ++i)
        if (&this->child(i) == &child)
            return i;
    return -1;
}

Widget *WidgetContainer::findChild(const char *name)
{
    WidgetTable::const_iterator it = m_childTable.find(name);
    if (it == m_childTable.end())
        return NULL;
    return it->second;
}

const Widget *WidgetContainer::findChild(const char *name) const
{
    WidgetTable::const_iterator it = m_childTable.find(name);
    if (it == m_childTable.end())
        return NULL;
    return it->second;
}

void WidgetContainer::addChildInternally(Widget &child)
{
    assert(!child.parent());
    child.setParent(this);
    m_childTable.insert(std::make_pair(child.name(), &child));
}

void WidgetContainer::removeChildInternally(Widget &child)
{
    assert(child.parent() == this);
    child.setParent(NULL);
    m_childTable.erase(child.name());
}

}
