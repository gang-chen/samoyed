// Widget.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_HPP
#define SMYD_WIDGET_HPP

#include <list>
#include <map>
#include <string>
#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/signals2/signal.hpp>
#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class WidgetContainer;

class Widget: public boost::noncopyable
{
public:
    typedef boost::signals2::signal<void (Widget &widget)> Unparent;

    /**
     * An XML element used to save and restore a widget.
     */
    class XmlElement: public boost::noncopyable
    {
    public:
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
        virtual xmlNodePtr write() const;

        /**
         * Create an XML element storing the information on a widget.
         */
        XmlElement(const Widget &widget);

        /**
         * Restore a widget and its child widgets from this XML element.  If
         * the widget cannot be restored to the saved state exactly, the XML
         * element will be changed to the restored state.
         * @return The restored widget, or NULL if failed.
         */
        virtual Widget *restoreWidget() = 0;

        const char *id() const { return m_id.c_str(); }
        const char *title() const { return m_title.c_str(); }
        const char *description() const { return m_description.c_str(); }
        bool visible() const { return m_visible; }

    protected:
        typedef
        boost::function<XmlElement *(xmlDocPtr doc,
                                     xmlNodePtr node,
                                     std::list<std::string> &errors)> Reader;

        static bool registerReader(const char *className,
                                   const Reader &reader);

        XmlElement(): m_visible(true) {}

        bool readInternally(xmlDocPtr doc,
                            xmlNodePtr node,
                            std::list<std::string> &errors);

    private:
        static std::map<std::string, Reader> s_readerRegistry;

        std::string m_id;
        std::string m_title;
        std::string m_description;
        bool m_visible;
    };

    /**
     * A widget is assigned an identifier, which is unique in its parent
     * container.
     * @return The identifier of the widget.
     */
    const char *id() const { return m_id.c_str(); }

    /**
     * @return The underlying GTK+ widget, which is owned by this widget.
     */
    GtkWidget *gtkWidget() const { return m_gtkWidget; }

    /**
     * @return The widget object that owns the GTK+ widget.
     */
    static Widget *getFromGtkWidget(GtkWidget *gtkWidget);

    /**
     * @return The title of the widget, which may be shown in a title bar.
     */
    const char *title() const { return m_title.c_str(); }
    void setTitle(const char *title);

    /**
     * @return A description of the widget, which may be shown in a tooltip.
     */
    const char *description() const { return m_description.c_str(); }
    void setDescription(const char *description);

    /**
     * @return The current widget in this container if this is a container, or
     * this widget.
     */
    virtual Widget &current() { return *this; }
    virtual const Widget &current() const { return *this; }

    WidgetContainer *parent() { return m_parent; }
    const WidgetContainer *parent() const { return m_parent; }

    void setParent(WidgetContainer *parent);

    bool closing() const { return m_closing; }

    virtual bool close() = 0;

    /**
     * Save the information on this widget in an XML element.
     */
    virtual XmlElement *save() const = 0;

    /**
     * Set this widget as the current widget.
     */
    void setCurrent();

    boost::signals2::connection
    addUnparentCallback(const Closed::slot_type &callback)
    { return m_unparent.connect(callback); }

protected:
    Widget(): m_gtkWidget(NULL), m_parent(NULL), m_closing(false) {}

    virtual ~Widget();

    bool setup(const char *id);

    bool restore(XmlElement &xmlElement);

    void setGtkWidget(GtkWidget *gtkWidget);

    void setClosing(bool closing) { m_closing = closing; }

private:
    GtkWidget *m_gtkWidget;

    std::string m_id;

    std::string m_title;

    std::string m_description;

    WidgetContainer *m_parent;

    bool m_closing;

    Unparent m_unparent;
};

}

#endif
