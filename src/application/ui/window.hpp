// Top-level window.
// Copyright (C) 2010 Gang Chen

#ifndef SMYD_WINDOW_HPP
#define SMYD_WINDOW_HPP

#include <gtk/gtk.h>

namespace Samoyed
{

/**
 * A window represents a top-level window.
 *
 * Windows are managed by the application.
 */
class Window
{
public:
    struct Configuration
    {
        int m_width;
        int m_height;
        int m_x;
        int m_y;
        bool m_fullScreen;
        bool m_maximized;
        bool m_toolbarVisible;
    };

    Configuration configuration() const;

    /**
     * @return The underlying GTK+ widget.  Note that it is read-only.
     */
    GtkWidget *gtkWidget() const { return m_window; }

    const Window *next() const { return m_next; }
    const Window *previous() const { return m_previous; }

private:
    static gboolean onDestroyEvent(GtkWidget *widget,
                                   GdkEvent *event,
                                   gpointer window);
    static void onDestroyed(GtkWidget *widget, gpointer window);
    static gboolean onFocusIn(GtkWidget *widget,
                              GdkEvent *event,
                              gpointer window);

    Window();

    ~Window();

    bool create(const Configuration *config);

    bool destroy();

    void addToList(Window *&windows);

    void removeFromList(Window *&windows);

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
    GtkActionGroup *m_projectActions;

    /**
     * The GTK+ actions that are sensitive when some files are opened.
     */
    GtkActionGroup *m_documentActions;

    /**
     * The next window in the application.
     */
    Window *m_next;

    /**
     * The previous window in the application.
     */
    Window *m_previous;

    friend class Application;
    friend class Actions;
};

}

#endif
