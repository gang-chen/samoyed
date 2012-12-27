// Pane base.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PANE_BASE_HPP
#define SMYD_PANE_BASE_HPP

#include <assert.h>
#include <boost/utility.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class Pane;
class SplitPane;
class Window;

class PaneBase: public boost::noncopyable
{
public:
    enum Type
    {
        TYPE_PROJECT_EXPLORER,
        TYPE_EDITOR_GROUP,
        TYPE_SPLIT_PANE
    };

    PaneBase(Type type):
        m_type(type),
        m_closing(false),
        m_window(NULL),
        m_parent(NULL)
    {}

    virtual ~PaneBase() {}

    virtual bool close() = 0;

    virtual GtkWidget *gtkWidget() const = 0;

    virtual Pane &currentPane() = 0;
    virtual const Pane &currentPane() const = 0;

    SplitPane *parent() { return m_parent; }
    const SplitPane *parent() const { return m_parent; }
    void setParent(SplitPane *parent)
    {
        assert(!m_window);
        assert((m_parent && !parent) || (!m_parent || parent));
        m_parent = parent;
    }

    Window *window() { return m_window; }
    const Window *window() const { return m_window; }
    void setWindow(Window *window)
    {
        assert(!m_parent);
        assert((m_window && !window) || (!m_window || window));
        m_window = window;
    }

    bool closing() const { return m_closing; }

    Type type() const { return m_type; }

protected:
    void setClosing(bool closing) { m_closing = closing; }

private:
    Type m_type;

    bool m_closing;

    Window *m_window;

    SplitPane *m_parent;
};

}

#endif
