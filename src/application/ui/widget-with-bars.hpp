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

/**
 * A widget with bars contains a main child and one or more bars around the main
 * child.
 */
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

        /**
         * Remove a bar.  When failing to create a bar, we need to remove the
         * XML element for the bar to keep the one-to-one map between the bars
         * and the XML elements.
         */
        void removeBar(int index);

        Widget::XmlElement &mainChild() const { return *m_mainChild; }
        int barCount() const { return m_bars.size(); }
        Bar::XmlElement &bar(int index) const { return *m_bars[index]; }
        int currentChildIndex() const { return m_currentChildIndex; }

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

        Widget::XmlElement *m_mainChild;
        std::vector<Bar::XmlElement *> m_bars;
        int m_currentChildIndex;
    };

    WidgetWithBars(const char *name, Widget &mainChild);

    virtual GtkWidget *gtkWidget() const { return m_verticalGrid; }

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void onChildClosed(const Widget &child);

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    virtual int childCount() const { return 1 + barCount(); }

    virtual Widget &child(int index)
    { return index == 1 ? *m_mainChild : *m_bars[index - 1]; }
    virtual const Widget &child(int index) const
    { return index == 1 ? *m_mainChild : *m_bars[index - 1]; }

    virtual int currentChildIndex() const { return m_currentChildIndex; }
    virtual void setCurrentChildIndex(int index)
    { m_currentChildIndex = index; }

    Widget &mainChild() { return *m_mainChild; }
    const Widget &mainChild() const { return *m_mainChild; }

    void addBar(Bar &bar);

    void removeBar(Bar &bar);

    int barCount() const { return m_bars.size(); }

    Bar &bar(int index) { return *m_bars[index]; }
    const Bar &bar(int index) const { return *m_bars[index]; }

    int barIndex(const Bar &bar) const;

protected:
    /**
     * @throw std::runtime_error if failing to create the widget.
     */
    WidgetWithBars(XmlElement &xmlElement);

    virtual ~WidgetWithBars();

    void addMainChild(Widget &child);

    void removeMainChild(Widget &child);

private:
    static void setFocusChild(GtkWidget *container,
                              GtkWidget *child,
                              gpointer widget);

    GtkWidget *m_verticalGrid;

    GtkWidget *m_horizontalGrid;

    Widget *m_mainChild;

    std::vector<Bar *> m_bars;

    int m_currentChildIndex;
};

}

#endif
