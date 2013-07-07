// View.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "view.hpp"
#include "view-extension.hpp"
#include "application.hpp"
#include "utilities/plugin-manager.hpp"
#include <list>
#include <string>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libxml/tree.h>

#define WIDGET "widget"
#define VIEW "view"
#define EXTENSION_ID "extension-id"

namespace Samoyed
{

void View::XmlElement::registerReader()
{
    Widget::XmlElement::registerReader(VIEW,
                                       Widget::XmlElement::Reader(read));
}

bool View::XmlElement::readInternally(xmlNodePtr node,
                                      std::list<std::string> &errors)
{
    char *value, *cp;
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
                g_free(cp);
                return false;
            }
            if (!Widget::XmlElement::readInternally(child, errors))
                return false;
            widgetSeen = true;
        }
        else if (strcmp(reinterpret_cast<const char *>(child->name),
                        EXTENSION_ID) == 0)
        {
            value = reinterpret_cast<char *>(
                xmlNodeGetContent(child->children));
            if (value)
            {
                m_extensionId = value;
                g_free(value);
            }
        }
    }
    if (m_extensionId.empty())
    {
        cp = g_strdup_printf(
            _("Line %d: \"%s\" element missing.\n"),
            node->line, EXTENSION_ID);
        errors.push_back(cp);
        g_free(cp);
        return false;
    }
    return true;
}

View::XmlElement *View::XmlElement::read(xmlNodePtr node,
                                         std::list<std::string> &errors)
{
    XmlElement *element = new XmlElement;
    if (!element->readInternally(node, errors))
    {
        delete element;
        return NULL;
    }
    return element;
}

xmlNodePtr View::XmlElement::write() const
{
    xmlNodePtr node = xmlNewNode(NULL,
                                 reinterpret_cast<const xmlChar *>(VIEW));
    xmlAddChild(node, Widget::XmlElement::write());
    xmlNewTextChild(node, NULL,
                    reinterpret_cast<const xmlChar *>(EXTENSION_ID),
                    reinterpret_cast<const xmlChar *>(extensionId()));
    return node;
}

View::XmlElement::XmlElement(const View &view):
    Widget::XmlElement(view),
    m_extensionId(view.extensionId())
{
}

Widget *View::XmlElement::restoreWidget()
{
    ViewExtension *ext =
        static_cast<ViewExtension *>(Samoyed::Application::instance().
                pluginManager().acquireExtension(extensionId()));
    if (!ext)
        return NULL;
    Widget *widget = ext->restoreView(*this);
    ext->release();
    return widget;
}

Widget::XmlElement *View::save() const
{
    return new XmlElement(*this);
}

}
