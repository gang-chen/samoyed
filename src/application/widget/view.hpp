// View.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_VIEW_HPP
#define SMYD_VIEW_HPP

#include "widget.hpp"
#include <list>
#include <string>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

/**
 * Views have two limitations:
 *
 *  1. When requested to be closed, views should be closed immediately.
 *
 *  2. Views cannot save their specific states to XML elements and restore.
 */
class View: public Widget
{
public:
    class XmlElement: public Widget::XmlElement
    {
    public:
        static void registerReader();

        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);
        virtual xmlNodePtr write() const;
        XmlElement(const View &view);
        virtual Widget *restoreWidget();

        const char *extensionId() const { return m_extensionId.c_str(); }

    protected:
        XmlElement() {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);

    private:
        std::string m_extensionId;
    };

    virtual Widget::XmlElement *save() const;

    const char *extensionId() const { return m_extensionId.c_str(); }

protected:
    View(const char *extensionId): m_extensionId(extensionId) {}

private:
    std::string m_extensionId;
};

}

#endif
