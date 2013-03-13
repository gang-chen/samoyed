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

        virtual ~XmlElement();
        virtual xmlNodePtr write() const;
        static XmlElement *saveWidget(const WidgetWithBars &widget);
        virtual Widget *restoreWidget();

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
        bool readInternally(xmlDocPtr doc,
                            xmlNodePtr node,
                            std::list<std::string> &errors);

        void saveWidgetInternally(const WidgetWithBars &widget);

        XmlElement(): m_mainChild(NULL), m_currentChildIndex(0) {}

    private:
        static Widget::XmlElement *read(xmlDocPtr doc,
                                        xmlNodePtr node,
                                        std::list<std::string> &errors);

        Widget::XmlElement *m_mainChild;
        std::vector<Bar::XmlElement *> m_bars;
        int m_currentChildIndex;
    };

    static WidgetWithBars *create(const char *name, Widget &mainChild);

    virtual bool close();

    virtual Widget::XmlElement *save() const;

    virtual void replaceChild(Widget &oldChild, Widget &newChild);

    virtual int childCount() const
    { return (m_mainChild ? 1 : 0) + barCount(); }

    virtual Widget &child(int index)
    { return index == 0 ? *m_mainChild : *m_bars[index - 1]; }
    virtual const Widget &child(int index) const
    { return index == 0 ? *m_mainChild : *m_bars[index - 1]; }

    virtual int currentChildIndex() const { return m_currentChildIndex; }
    virtual void setCurrentChildIndex(int index)
    { m_currentChildIndex = index; }

    Widget &mainChild() { return *m_mainChild; }
    const Widget &mainChild() const { return *m_mainChild; }

    void addBar(Bar &bar) { addBarInternally(bar); }

    void removeBar(Bar &bar) { removeChild(bar); }

    int barCount() const { return m_bars.size(); }

    Bar &bar(int index) { return *m_bars[index]; }
    const Bar &bar(int index) const { return *m_bars[index]; }

    int barIndex(const Bar &bar) const;

protected:
    WidgetWithBars():
        m_horizontalGrid(NULL),
        m_mainChild(NULL),
        m_currentChildIndex(0)
    {}

    virtual ~WidgetWithBars();

    bool setup(const char *name, Widget &mainChild);

    bool restore(XmlElement &xmlElement);

    void addMainChild(Widget &child);

    void removeMainChild(Widget &child);

    void addBarInternally(Bar &bar);

    void removeBarInternally(Bar &bar);

    virtual void removeChildInternally(Widget &child);

private:
    static void setFocusChild(GtkWidget *container,
                              GtkWidget *child,
                              gpointer widget);

    GtkWidget *m_horizontalGrid;

    Widget *m_mainChild;

    std::vector<Bar *> m_bars;

    int m_currentChildIndex;
};

}

#endif
