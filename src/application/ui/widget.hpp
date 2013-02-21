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
        XmlElement(const Widget &widget): m_name(widget.name()) {}

        virtual ~XmlElement() {}

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

        const char *name() const { return m_name.c_str(); }

    protected:
        typedef
        boost::function<Widget::XmlElement *(xmlDocPtr doc,
                                             xmlNodePtr node,
                                             std::list<std::string> &errors)>
            Reader;

        static bool registerReader(const char *className,
                                   const Reader &reader);

        XmlElement(xmlDocPtr doc,
                   xmlNodePtr node,
                   std::list<std::string> &errors);

    private:
        static std::map<std::string, Reader> s_readerRegistry;

        std::string m_name;
    };

    Widget(const char *name): m_name(name), m_parent(NULL), m_closing(false) {}

    /**
     * A widget may be assigned a unique name, which is used to identify it.
     * @return The name of the widget.
     */
    const char *name() const { return m_name.c_str(); }

    /**
     * @return The title of the widget, which may be shown in a title bar.
     */
    const char *title() const { return m_title.c_str(); }

    /**
     * @return A short description of the widget, which may be shown in a
     * tooltip.
     */
    const char *description() const { return m_description.c_str(); }

    void setTitle(const char *title) { m_title = title; }

    void setDescription(const char *description)
    { m_description = description; }

    /**
     * @return The underlying GTK+ widget.  Note that it is read-only.
     */
    virtual GtkWidget *gtkWidget() const = 0;

    /**
     * @return The current widget in this container if this is a container, or
     * this widget.
     */
    virtual Widget &current() { return *this; }
    virtual const Widget &current() const { return *this; }

    WidgetContainer *parent() const { return m_parent; }

    void setParent(WidgetContainer *parent) { m_parent = parent; }

    bool closing() const { return m_closing; }

    virtual bool close() = 0;

    /**
     * Save the configuration, state and history of this widget in an XML
     * element.
     */
    virtual XmlElement *save() const;

    /**
     * Set this widget as the current widget.
     */
    void setCurrent();

protected:
    virtual ~Widget();

    void setClosing(bool closing) { m_closing = closing; }

private:
    const std::string m_name;

    std::string m_title;

    std::string m_description;

    WidgetContainer *m_parent;

    bool m_closing;
};

}

#endif
