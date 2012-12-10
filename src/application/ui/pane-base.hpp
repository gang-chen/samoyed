// Pane base.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PANE_BASE_HPP
#define SMYD_PANE_BASE_HPP

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

    PaneBase(Type type): m_type(type), m_parent(NULL), m_window(NULL) {}
    virtual ~PaneBase();

    virtual GtkWidget *gtkWidget() const = 0;

    virtual Pane &currentPane() = 0;
    virtual const Pane &currentPane() const = 0;

    SplitPane *parent() { return m_parent; }
    const SplitPane *parent() const { return m_parent; }
    void setParent(SplitPane *parent) { m_parent = parent; }

    Window *window() { return m_window; }
    const Window *window() const { return m_window; }
    void setWindow(Window *window) { m_window = window; }

    virtual bool close() = 0;

    Type type() const { return m_type; }

private:
    Type m_type;
    SplitPane *m_parent;
    Window *m_window;
};

}

#endif
