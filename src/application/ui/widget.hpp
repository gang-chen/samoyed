// Widget.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_HPP
#define SMYD_WIDGET_HPP

#include <list>
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class WidgetContainer;

class Widget
{
public:
    /**
     * An XML element used to save and restore a widget.
     */
    class XmlElement
    {
    public:
        typedef
        boost::function<Widget::XmlElement *(xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &errors)>
            Reader;

        virtual ~XmlElement() {}

        static bool registerReader(const char *className,
                                   const Reader &reader);

        /**
         * Read the information from an XML node and store it in an XML element.
         * @param doc The input XML document.
         * @param node The input XML node.
         * @param errors The read errors.
         * @return The created XML element storing the read information, or NULL
         * if any error is found.
         */
        static XmlElement *read(xmlDocPtr doc,
                                xmlNodePtr node,
                                std::list<std::string> &errors);

        /**
         * Write to an XML node.
         */
        virtual xmlNodePtr write() const = 0;

        /**
         * Create a widget, its child widgets and bars from this XML element.
         * @return The created widget, or NULL if failed.
         */
        virtual Widget *createWidget() = 0;

        /**
         * Restore the widget, its child widgets and bars.
         * @param widget The widget created from this XML element.
         * @return True if successful.
         */
        virtual bool restoreWidget(Widget &widget) const = 0;

    private:
        static std::map<std::string, Reader> s_readerRegistry;
    };

    /**
     * @return The underlying GTK+ widget.  Note that it is read-only.
     */
    virtual GtkWidget *gtkWidget() const = 0;

    virtual const char *title() const { return ""; }

    virtual const char *description() const { return ""; }

    virtual Widget &current() { return *this; }
    virtual const Widget &current() const { return *this; }

    WidgetContainer *parent() const { return m_parent; }

    void setParent(WidgetContainer *parent) { m_parent = parent; }

    bool closing() const { return m_closing; }

    virtual bool close() = 0;

    /**
     * Save the configuration of this widget in an XML element.
     */
    virtual XmlElement *save() const = 0;

protected:
    Widget(): m_parent(NULL), m_closing(false) {}

    virtual ~Widget();

    void setClosing(bool closing) { m_closing = closing; }

private:
    WidgetContainer *m_parent;

    bool m_closing;
};

}

#endif
