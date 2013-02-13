// Widget with bars.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_WITH_BARS_HPP
#define SMYD_WIDGET_WITH_BARS_HPP

#include "widget-container.hpp"
#include "bar.hpp"
#include <list>
#include <string>
#include <vector>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class WidgetWithBars: public WidgetContainer
{
public:
    class XmlElement: public Widget::XmlElement
    {
    public:
        static bool registerReader();

        XmlElement(const WidgetWithBars &widget);
        virtual ~XmlElement();
        virtual xmlNodePtr write() const;
        virtual Widget *createWidget();
        virtual bool restoreWidget(Widget &widget) const;

    protected:
        /**
         * @throw std::runtime_error if any fatal error is found in the input
         * XML file.
         */
        XmlElement(xmlDocPtr doc,
                   xmlNodePtr node,
                   std::list<std::string> &errors);

    private:
        static Widget::XmlElement *read(xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors);

        Widget::XmlElement *m_child;
        std::vector<Bar::XmlElement *> m_bars;
    };

    WidgetWithBars();

    /**
     * @throw std::runtime_error if failing to create the widget.
     */
    WidgetWithBars(XmlElement &xmlElement);

    virtual ~WidgetWithBars();

    virtual GtkWidget *gtkWidget() const { return m_grid; }

    virtual Widget &current() { return m_child->current(); }
    virtual const Widget &current() const { return m_child->current(); }

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void onChildClosed(const Widget *child);

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    Widget &child() { return *m_child; }
    const Widget &child() const { return *m_child; }

    void onBarClosed(const Bar *bar);

protected:
    void addChild(Widget &child);

    void removeChild(Widget &child);

private:
    GtkWidget *m_grid;

    Widget *m_child;

    std::vector<Bar *> m_bars;
};

}

#endif
