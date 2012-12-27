// Top-level window.
// Copyright (C) 2010 Gang Chen

#ifndef SMYD_WINDOW_HPP
#define SMYD_WINDOW_HPP

#include "../utilities/misc.hpp"
#include <boost/utility.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class PaneBase;
class Temporary;

/**
 * A window represents a top-level window.
 */
class Window: public boost::noncopyable
{
public:
    struct Configuration
    {
        int m_screenIndex;
        int m_x;
        int m_y;
        int m_width;
        int m_height;
        bool m_fullScreen;
        bool m_maximized;
        bool m_toolbarVisible;
        Configuration():
            m_screenIndex(-1),
            m_fullScreen(false),
            m_maximized(true),
            m_toolbarVisible(true)
        {}
    };

    Window(const Configuration &config, PaneBase &content);

    bool close();

    void onContentClosed();

    Configuration configuration() const;

    PaneBase &content() { return *m_content; }
    const PaneBase &content() const { return *m_content; }

    void setContent(PaneBase *content);

    void addTemporary(Temporary &temp);
    void removeTemporary(Temporary &temp);

    /**
     * @return The underlying GTK+ widget.  Note that it is read-only.
     */
    GtkWidget *gtkWidget() const { return m_window; }

private:
    static gboolean onDeleteEvent(GtkWidget *widget,
                                  GdkEvent *event,
                                  gpointer window);
    static void onDestroy(GtkWidget *widget, gpointer window);
    static gboolean onFocusInEvent(GtkWidget *widget,
                                   GdkEvent *event,
                                   gpointer window);

    ~Window();

    PaneBase *m_content;

    Temporary *m_firstTemp;
    Temporary *m_lastTemp;

    bool m_closing;

    /**
     * The GTK+ window.
     */
    GtkWidget *m_window;

    GtkWidget *m_mainVBox;

    GtkWidget *m_mainHBox;

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

    SMYD_DEFINE_DOUBLY_LINKED(Window)
};

}

#endif
