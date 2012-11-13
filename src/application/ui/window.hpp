// Top-level window.
// Copyright (C) 2010 Gang Chen

#ifndef SMYD_WINDOW_HPP
#define SMYD_WINDOW_HPP

#include <boost/utility>
#include <gtk/gtk.h>

namespace Samoyed
{

/**
 * A window represents a top-level window.
 */
class Window: public boost::noncopyable
{
public:
    struct Configuration
    {
        int m_x;
        int m_y;
        int m_width;
        int m_height;
        bool m_fullScreen;
        bool m_maximized;
        bool m_toolbarVisible;
    };

    static Window *create(const Configuration *config);

    ~Window();

    Configuration configuration() const;

    /**
     * @return The underlying GTK+ widget.  Note that it is read-only.
     */
    GtkWidget *gtkWidget() const { return m_window; }

private:
    static gboolean onDeleteEvent(GtkWidget *widget,
                                  GdkEvent *event,
                                  gpointer);

    Window();

    /**
     * The GTK+ window.
     */
    GtkWidget *m_window;

    GtkWidget *m_mainBox;

    GtkWidget *m_menuBar;

    GtkWidget *m_toolbar;

    /**
     * The GTK+ UI manager that creates the menu bar, toolbar and popup menu in
     * this window.
     */
    GtkUIManager *m_uiManager;

    /**
     * The GTK+ actions that are always sensitive.
     */
    GtkActionGroup *m_basicActions;

    /**
     * The GTK+ actions that are sensitive when some projects are opened.
     */
    GtkActionGroup *m_actionsForProjects;

    /**
     * The GTK+ actions that are sensitive when some files are opened.
     */
    GtkActionGroup *m_actionsForFiles;
};

}

#endif
