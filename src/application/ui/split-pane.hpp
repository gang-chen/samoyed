// Split pane.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_SPLIT_PANE_HPP
#define SMYD_SPLIT_PANE_HPP

#include "pane-base.hpp"
#include <gtk/gtk.h>

namespace Samoyed
{

class Pane;

class SplitPane: public PaneBase
{
public:
    enum Orientation
    {
        ORIENTATION_HORIZONTAL = GTK_ORIENTATION_HORIZONTAL,
        ORIENTATION_VERTICAL = GTK_ORIENTATION_VERTICAL
    };

    SplitPane(Orientation orientation, PaneBase &child1, PaneBase &child2);

    virtual ~SplitPane();

    virtual GtkWidget *gtkWidget() const { return m_paned; }

    virtual Pane *currentPane()
    { return m_children[m_currentIndex]->currentPane(); }

    virtual const Pane *currentPane() const
    { return m_children[m_currentIndex]->currentPane(); }

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int currentIndex) { m_currentIndex = currentIndex; }

    PaneBase *child(int index) { return m_children[index]; }
    const PaneBase *child(int index) const { return m_children[index]; }

    virtual bool close();

    void replaceChild(PaneBase &oldChild, PaneBase &newChild);

    void removeChild(PaneBase &child);

private:
    static void onDestroy(GtkWidget *widget, gpointer splitPane);

    PaneBase *m_children[2];

    int m_currentIndex;

    GtkWidget *m_paned;
};

}

#endif
