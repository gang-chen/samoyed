// Split pane.
// Copyright (C) 2012 Gang Chen.

#include "split-pane.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

SplitPane::SplitPane(Orientation orientation,
                     PaneBase &child1,
                     PaneBase &child2):
    m_currentIndex(0)
{
    m_children[0] = &child1;
    m_children[1] = &child2;
    m_paned = gtk_paned_new(orientation);
    gtk_paned_add1(GTK_PANED(m_paned), child1.gtkWidget());
    gtk_paned_add2(GTK_PANED(m_paned), child2.gtkWidget());
}

SplitPane::~SplitPane()
{
    if (m_paned)
    {
        GtkWidget *w = m_paned;
        m_paned = NULL;
        gtk_widget_destroy(w);
    }
}

bool SplitPane::close()
{
    int currentIndex = m_currentIndex;
    if (!m_children[currentIndex]->close())
        return false;
    if (!m_children[1 - currentIndex]->close())
        return false;
    return true;
}

void SplitPane::replaceChild(PaneBase &oldChild, PaneBase &newChild)
{
    int index = &oldChild == m_children[0] ? 0 : 1;
    gtk_container_remove(GTK_CONTAINER(m_paned), oldChild.gtkWidget());
    if (index == 0)
        gtk_container_add1(GTK_PANED(m_paned), newChild.gtkWidget());
    else
        gtk_container_add2(GTK_PANED(m_paned), newChild.gtkWidget());
}

void SplitPane::removeChild(PaneBase &child)
{
    gtk_container_remove(GTK_CONTAINER(m_paned), child.gtkWidget());

    // If a child is removed, this split pane should be removed as well and
    // replaced by the remained child.
    PaneBase *other = &child == m_children[0] ? m_children[1] : m_children[0];
    other->setParent(parent());
    g_object_ref(other->gtkWidget());
    if (parent())
        parent()->replaceChild(*this, *other);
    else
    {
        assert(window()->pane() == this);
        window()->setPane(other);
    }
    g_object_unref(other->gtkWidget());
}

void SplitPane::onDestroy(GtkWidget *widget, gpointer splitPane)
{
    SplitPane *s = static_cast<SplitPane *>(splitPane);
    if (s->m_paned)
    {
        assert(s->m_paned == widget);
        s->m_paned = NULL;
        delete s;
    }
}

}
