// Splash screen.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_SPLASH_SCREEN_HPP
#define SMYD_SPLASH_SCREEN_HPP

#include <boost/utility.hpp>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

namespace Samoyed
{

class SplashScreen: public boost::noncopyable
{
public:
    SplashScreen(const char *title, const char *imageFileName);
    ~SplashScreen();
    void setProgress(double progress, const char *message);

private:
    static gboolean drawImage(GtkWidget *widget,
                              cairo_t *cr,
                              gpointer splash);
    static gboolean onDestroyEvent(GtkWidget *widget,
                                   GdkEvent *event,
                                   gpointer data);

    GtkWidget *m_window;
    GtkWidget *m_progressBar;
    GdkPixbuf *m_image;
};

}

#endif
