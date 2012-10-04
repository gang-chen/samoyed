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
        int width;
        int height;
        int x;
        int y;
        bool fullScreen;
        bool maximized;
        bool toolbarVisible;
        bool sidePanelVisible;
        int sidePanelWidth;
    };

    Configuration configuration() const;

    GtkWidget *gtkWidget() { return m_window; }

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

    friend class Application;
    friend class Actions;
};

}

#endif
