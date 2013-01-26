// Widget.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_WIDGET_HPP
#define SMYD_WIDGET_HPP

#include <gtk/gtk.h>
#include <libxml/tree.h>

namespace Samoyed
{

class Widget
{
public:
    class XmlElement
    {
    public:
        virtual ~XmlElement() {}
        virtual xmlNodePtr write() const = 0;
        virtual bool restore(Samoyed::PaneBase *&pane) const = 0;
        static XmlElementPaneBase *save(const Samoyed::PaneBase &pane);
    };

    virtual GtkWidget *gtkWidget() const = 0;

    bool closing() const { return m_closing; }

    virtual bool close() = 0;

protected:
    virtual ~Widget() {}

    void setClosing(bool closing) { m_closing = closing; }

private:
    bool m_closing;
};

}

#endif
