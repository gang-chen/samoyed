// Split pane.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "split-pane.hpp"
#include <assert.h>
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
    assert(!window() && !parent());
    assert(!m_children[0] && !m_children[1]);
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
    assert((window() && !parent()) || (!window() && parent()));

    // Mark the child being removed.
    int index = &child == m_children[0] ? 0 : 1;
    assert(m_children[index]);
    m_children[index] = NULL;

    // Remove the remained child from this split pane.
    PaneBase *remained = m_children[1 - index];
    assert(remained);
    g_object_ref(remained->gtkWidget());
    removeChild(*remained);

    // Keep the GTK+ widget alive.  We will destroy it by ourselves.
    g_object_ref(gtkWidget());

    // Replace this split pane with the remained child.
    if (window())
    {
        window()->setContent(NULL);
        window()->setContent(remained);
    }
    else if (parent())
    {
        index = this == parent()->child(0) ? 0 : 1;
        parent()->removeChild(*this);
        parent()->addChild(*remained, index);
    }
    g_object_unref(remained->gtkWidget());

    // Destroy this split pane.
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
    m_children[index] = NULL;
    child.setParent(NULL);
    gtk_container_remove(GTK_CONTAINER(m_paned), child.gtkWidget());
}

}
