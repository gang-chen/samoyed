// Paned widget.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "paned.hpp"
#include <assert.h>
#include <gtk/gtk.h>
#include <libxml/xmlstring.h>

namespace Samoyed
{

Paned::Paned(Orientation orientation, int position,
             Widget &child1, Widget &child2):
    m_orientation(orientation),
    m_currentChildIndex(0)
{
    m_children[0] = &child1;
    m_children[1] = &child2;
    m_paned = gtk_paned_new(static_cast<GtkOrientation>(orientation));
    gtk_paned_add1(GTK_PANED(m_paned), child1.gtkWidget());
    gtk_paned_add2(GTK_PANED(m_paned), child2.gtkWidget());
    if (position != -1)
        gtk_paned_set_position(GTK_PANED(m_paned), position);
}

Paned::~Paned()
{
    assert(!m_children[0] && !m_children[1]);
    gtk_widget_destroy(m_paned);
}

bool Paned::close()
{
    if (closing())
        return true;

    setClosing(true);
    Widget *child1 = m_children[m_currentIndex];
    Widget *child2 = m_children[1 - m_currentIndex];
    if (!child1->close())
    {
        setClosing(false);
        return false;
    }
    if (!child2->close())
        // Do not call setClosing(false) because the paned container will be
        // deleted if either child is closed.
        return false;
    return true;
}

void Paned::addChild(Widget &child, int index)
{
    assert(!m_children[index]);
    assert(!child.parent());
    m_children[index] = &child;
    child.setParent(this);
    if (index == 0)
        gtk_paned_add1(GTK_PANED(m_paned), child.gtkWidget());
    else
        gtk_paned_add2(GTK_PANED(m_paned), child.gtkWidget());
}

void Paned::removeChild(Widget &child)
{
    int index = childIndex(child);
    assert(child.parent() == this);
    assert(!child.window());
    m_children[index] = NULL;
    child.setParent(NULL);
    gtk_container_remove(GTK_CONTAINER(m_paned), child.gtkWidget());
}

void Paned::onChildClosed(Widget *child)
{
    // Mark the child as being removed.
    int index = childIndex(child);
    m_children[index] = NULL;

    // Remove the remained child from this paned container.
    Widget *remained = m_children[1 - index];
    assert(remained);
    g_object_ref(remained->gtkWidget());
    removeChild(*remained);

    // Keep the GTK+ widget alive.  We will destroy it by ourselves.
    g_object_ref(gtkWidget());

    // Replace this paned container with the remained child.
    assert(parent());
    index = parent()->childIndex(*this);
    parent()->removeChild(*this);
    parent()->addChild(*remained, index);

    // Destroy this paned container.
    delete this;
}

Paned *Paned::split(Orientation orientation, int position,
                    Widget &child1, Widget &child2)
{
    Widget *original;
    WidgetContainer *parent;
    int index;

    if (child1.parent())
        original = &child1;
    else
    {
        assert(child2.parent());
        original = &child2;
    }
    parent = original->parent();
    index = parent->childIndex(original);

    // Remove the original widget from its container.
    g_object_ref(original->gtkWidget());
    parent->removeChild(*original);

    // Create a paned container to hold the two widgets.
    Paned *paned = new Paned(orientation, position, child1, child2);

    // Add it to the container.
    parent->addChild(*paned, index);

    return paned;
}

int Paned::childIndex(const Widget *child) const
{
    if (m_children[0] == child)
        return 0;
    if (m_children[1] == child)
        return 1;
    return -1;
}

}
