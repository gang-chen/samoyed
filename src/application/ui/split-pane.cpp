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
    g_signal_connect(m_paned,
                     "destroy",
                     G_CALLBACK(onDestroy),
                     this);
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
    closing() = true;
    // Note that the split pane will be automatically closed after either of its
    // children is closed.
    return true;
}

void SplitPane::replaceChild(PaneBase &oldChild, PaneBase &newChild)
{
    int index = &oldChild == m_children[0] ? 0 : 1;
    gtk_container_remove(GTK_CONTAINER(m_paned), oldChild.gtkWidget());
    if (index == 0)
        gtk_paned_add1(GTK_PANED(m_paned), newChild.gtkWidget());
    else
        gtk_paned_add2(GTK_PANED(m_paned), newChild.gtkWidget());
}

void SplitPane::onChildClosed(PaneBase *child)
{
    // If a child is closed, this split pane should be destroyed and replaced by
    // the remained child.
    PaneBase *other = child == m_children[0] ? m_children[1] : m_children[0];
    other->setParent(parent());
    g_object_ref(other->gtkWidget());
    if (window())
        window()->setContent(other);
    else if (parent())
        parent()->replaceChild(*this, *other);
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
