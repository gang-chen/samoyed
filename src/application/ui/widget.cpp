// Widget.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "widget.hpp"
#include "widget-container.hpp"
#include <list>
#include <map>
#include <string>
#include <utility>
#include <libxml/tree.h>

namespace Samoyed
{

std::map<std::string, Widget::XmlElement::Reader>
    Widget::XmlElement::s_widgetRegistry;

bool Widget::XmlElement::registerWidget(const char *className,
                                        const Reader &reader)
{
    return s_readerRegistry.insert(std::make_pair(std::string(className),
                                                  reader)).second;
}

Widget::XmlElement* Widget::XmlElement::read(xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &errors)
{
    std::string className(reinterpret_cast<const char *>(node->name));
    std::map<std::string, Reader>::const_iterator it =
        s_readerRegistry.find(className);
    if (it == s_readerRegistry.end())
    {
        // Silently ignore the unknown widget class.
        return NULL;
    }
    return it->second(doc, node, errors);
}

Widget::~Widget()
{
    if (m_parent)
        m_parent->onChildClosed(this);
}

}
