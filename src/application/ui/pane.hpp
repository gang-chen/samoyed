// Pane.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PANE_HPP
#define SMYD_PANE_HPP

#include <boost/utility>
#include <gtk/gtk.h>

namespace Samoyed
{

class Window;

/**
 * A pane is a container.  Panes are organized in a binary tree.
 */
class Pane: public boost::noncopyable
{
public:
    enum ContentType
    {
        EDITOR_GROUP,
        PROJECT_EXPLORER
    };

private:
    ContentType m_contentType;
    void *m_content;

    Pane *m_left;
    Pane *m_right;

    Pane *m_parent;

    Window *m_window;

    GtkWidget *m_paned;
};

}

#endif
