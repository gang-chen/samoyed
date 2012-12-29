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
                     int position,
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
    if (position != -1)
        gtk_paned_set_position(m_paned, position);
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

    // Mark the child as being removed.
    int index = childIndex(child);
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
        index = parent()->childIndex(*this);
        parent()->removeChild(*this);
        parent()->addChild(*remained, index);
    }

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
    int index = childIndex(child);
    assert(child.parent() == this);
    assert(!child.window());
    m_children[index] = NULL;
    child.setParent(NULL);
    gtk_container_remove(GTK_CONTAINER(m_paned), child.gtkWidget());
}

SplitPane *SplitPane::split(Orientation orientation,
                            int position,
                            PaneBase &child1,
                            PaneBase &child2)
{
    PaneBase *originalPane, *newPane;
    Window *window;
    SplitPane *parent;
    int index;

    if (child1.window() || child1.parent())
    {
        originalPane = &child1;
        newPane = &child2;
    }
    else
    {
        assert(child2.window() || child2.parent());
        originalPane = &child2;
        newPane = &child1;
    }
    window = originalPane->window();
    parent = originalPane->parent();
    if (parent)
        index = parent->childIndex(*originalPane);

    // Remove the original pane from its container.
    g_object_ref(originalPane->gtkWidget());
    if (window)
        window->setContent(NULL);
    else
        parent->removeChild(*originalPane);

    // Create a split pane to hold the two panes.
    SplitPane *splitPane = new SplitPane(orientation, position, child1, child2);

    // Add it to the container.
    if (window)
        window->setContent(splitPane);
    else
        parent->addChild(*splitPane, index);

    return splitPane;
}

}
