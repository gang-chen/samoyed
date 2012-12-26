// Split pane.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "split-pane.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

SplitPane::SplitPane(Orientation orientation,
                     int length;
                     PaneBase &child1,
                     PaneBase &child2):
    m_orientation(orientation),
    m_currentIndex(0)
{
    m_children[0] = &child1;
    m_children[1] = &child2;
    m_paned = gtk_paned_new(orientation);
    gtk_paned_add1(GTK_PANED(m_paned), child1.gtkWidget());
    gtk_paned_add2(GTK_PANED(m_paned), child2.gtkWidget());
    if (length != -1)
        gtk_paned_set_position(m_paned, length);
}

SplitPane::~SplitPane()
{
    gtk_widget_destroy(m_paned);
}

bool SplitPane::close()
{
    setClosing(true);
    PaneBase *child1 = m_children[m_currentIndex];
    PaneBase *child2 = m_children[1 - m_currentIndex];
    if (!child1->close())
    {
        setClosing(false);
        return false;
    }
    if (!child2->close())
        return false;
    return true;
}

void SplitPane::onChildClosed(PaneBase *child)
{
    // If a child is closed, this split pane should be destroyed and replaced by
    // the remained child.
    int index = &child == m_children[0] ? 0 : 1;
    assert(m_children[index]);
    m_children[index] = NULL;

    PaneBase *other = m_children[1 - index];
    assert(other);
    g_object_ref(other->gtkWidget());
    removeChild(*other);

// remove first, add next; if remove cause chain reaction?
    assert((window() && !parent()) || (!window() && parent()));
    if (window())
    {
        window()->setContent(other);
        other->setWindow(window());
    }
    else if (parent())
    {
        index = parent()->child(0) == this ? 0 : 1;
        parent()->addChild(*other, index);
    }
    g_object_unref(other->gtkWidget());

    delete this;
}

void SplitPane::addChild(PaneBase &child, int index)
{
    assert(!m_children[index]);
    assert(!child.parent());
    assert(!child.window());
    m_children[index] = &child;
    child.setParent(this);
    if (index == 0)
        gtk_paned_add1(GTK_PANED(m_paned), child.gtkWidget());
    else
        gtk_paned_add2(GTK_PANED(m_paned), child.gtkWidget());
}

void SplitPane::removeChild(PaneBase &child)
{
    int index = &child == m_children[0] ? 0 : 1;
    assert(m_children[index] == &child);
    assert(child.parent() == this);
    assert(!child.window());
    gtk_container_remove(GTK_CONTAINER(m_paned), child.gtkWidget());
    m_children[index] = NULL;
    child.setParent(NULL);
}

}
