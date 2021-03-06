// Widget.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_HPP
#define SMYD_WIDGET_HPP

#include "window/actions.hpp"
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
class Window;

/**
 * A widget is owned by its parent widget if it is not a top-level widget.  To
 * destroy a widget, the user cannot destroy it directly but should close it.
 * When requested to be closed, a widget possibly asks the user to confirm the
 * close request, possibly refuses the close request if the user refuses,
 * possibly saves its working data, which may be completed or aborted
 * asynchronously, requests its child widgets, if any, to close, which may be
 * refused, and requests its owner to destroy itself.  When requested to destroy
 * a child widget, a widget removes the child widget, destroys the child, checks
 * to see if it has been requested to be closed and all child widgets are
 * destroyed, and if true, requests its owner to destroy it.
 */
class Widget: public boost::noncopyable
{
public:
    typedef boost::signals2::signal<void (Widget &widget)> Closed;

    typedef std::map<std::string, std::string> PropertyMap;

    /**
     * An XML element used to save and restore a widget.
     */
    class XmlElement: public boost::noncopyable
    {
    public:
        typedef
        boost::function<XmlElement *(xmlNodePtr node,
                                     std::list<std::string> *errors)> Reader;

        static void registerReader(const char *className,
                                   const Reader &reader);
        static void unregisterReader(const char *className);

        virtual ~XmlElement() {}

        /**
         * Read the information from an XML node and store it in an XML element.
         * @param doc The input XML document.
         * @param node The input XML node.
         * @param errors The read errors.
         * @return The created XML element storing the read information, or NULL
         * if any error is found.
         */
        static XmlElement *read(xmlNodePtr node,
                                std::list<std::string> *errors);

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
        const PropertyMap &properties() const { return m_properties; }

        void setTitle(const char *title) { m_title = title; }

    protected:
        XmlElement(): m_visible(true) {}

        bool readInternally(xmlNodePtr node, std::list<std::string> *errors);

    private:
        static std::map<std::string, Reader> s_readerRegistry;

        std::string m_id;
        std::string m_title;
        std::string m_description;
        bool m_visible;
        PropertyMap m_properties;
    };

    /**
     * This function can be called by the owner of this widget only.
     */
    virtual ~Widget();

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

    void setParent(WidgetContainer *parent) { m_parent = parent; }

    bool closing() const { return m_closing; }

    /**
     * Close this widget.  When this function returns, the caller can check to
     * see if the close request is refused.  If not refused, the close operation
     * may be already completed, or be ongoing.  If ongoing, the close operation
     * will be completed or canceled in the future.  To get notified of the
     * completion of the close operation, the caller can add a callback to the
     * closed signal.
     * @return False iff the widget refuses to close.
     */
    virtual bool close();

    /**
     * Cancel the close request.
     */
    virtual void cancelClosing();

    /**
     * This is function is called to notify the observers before it is destroyed
     * so that the observers can access the intact concrete widget.
     */
    void onClosed() { m_closed(*this); }

    /**
     * Save the information on this widget in an XML element.
     */
    virtual XmlElement *save() const = 0;

    virtual void grabFocus()
    { gtk_widget_grab_focus(gtkWidget()); }

    /**
     * Set this widget as the current widget.
     */
    void setCurrent();

    boost::signals2::connection
    addClosedCallback(const Closed::slot_type &callback)
    { return m_closed.connect(callback); }

    const std::string *getProperty(const char *name) const;
    void setProperty(const char *name, const std::string &value);
    const PropertyMap &properties() const { return m_properties; }

    virtual void activateAction(Window &window,
                                GtkAction *action,
                                Actions::ActionIndex index)
    {}

    virtual bool isActionSensitive(Window &window,
                                   GtkAction *action,
                                   Actions::ActionIndex index)
    { return false; }

protected:
    Widget(): m_gtkWidget(NULL), m_parent(NULL), m_closing(false) {}

    bool setup(const char *id);

    bool restore(XmlElement &xmlElement);

    void setGtkWidget(GtkWidget *gtkWidget);

    void setClosing(bool closing) { m_closing = closing; }

    /**
     * Destroy the widget by requesting the owner of the widget to destroy it.
     */
    virtual void destroy();

private:
    GtkWidget *m_gtkWidget;

    std::string m_id;

    std::string m_title;

    std::string m_description;

    WidgetContainer *m_parent;

    bool m_closing;

    Closed m_closed;

    PropertyMap m_properties;
};

}

#endif
