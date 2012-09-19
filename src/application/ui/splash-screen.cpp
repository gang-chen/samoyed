// Splash screen.
// Copyright (C) 2011 Gang Chen.

/*
g++ splash-screen.cpp -DSMYD_SPLASH_SCREEN_UNIT_TEST\
 `pkg-config --cflags --libs gtk+-3.0` -Wall -o splash-screen
*/
 
#include "splash-screen.hpp"
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#ifdef SMYD_SPLASH_SCREEN_UNIT_TEST
# include <stdio.h>
#endif

namespace Samoyed
{

SplashScreen::SplashScreen(const char *title, const char *imageFileName)
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    m_progressBar = gtk_progress_bar_new();
    m_image = gdk_pixbuf_new_from_file(imageFileName, NULL);
    if (m_image)
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(m_window), box);
        GtkWidget *draw = gtk_drawing_area_new();
        int imageWidth = gdk_pixbuf_get_width(m_image);
        int imageHeight = gdk_pixbuf_get_height(m_image);
        gtk_widget_set_size_request(draw, imageWidth, imageHeight);
        g_signal_connect(draw,
                         "draw",
                         G_CALLBACK(drawImage),
                         this);
        gtk_box_pack_start(GTK_BOX(box), draw, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), m_progressBar, FALSE, FALSE, 0);
        gtk_widget_show_all(box);
    }
    else
    {
        // Do not show the error message.
        gtk_container_add(GTK_CONTAINER(m_window), m_progressBar);
        gtk_widget_show(m_progressBar);
    }
    gtk_window_set_decorated(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_position(GTK_WINDOW(m_window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_title(GTK_WINDOW(m_window), title);
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(m_progressBar), TRUE);
    g_signal_connect(m_window,
                     "destroy-event",
                     G_CALLBACK(onDestroyEvent),
                     NULL);

    gtk_widget_show(m_window);
}

SplashScreen::~SplashScreen()
{
    gtk_widget_destroy(m_window);
    if (m_image)
        g_object_unref(m_image);
}

void SplashScreen::setProgress(double progress, const char *message)
{
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_progressBar), progress);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_progressBar), message);
}

gboolean SplashScreen::drawImage(GtkWidget *widget,
                                 cairo_t *cr,
                                 gpointer splash)
{
    SplashScreen *s = static_cast<SplashScreen *>(splash);
    gdk_cairo_set_source_pixbuf(cr, s->m_image, 0, 0);
    cairo_paint(cr);
    return TRUE;
}

gboolean SplashScreen::onDestroyEvent(GtkWidget *widget,
                                      GdkEvent *event,
                                      gpointer data)
{
    // Disable destroying the splash screen.
    return TRUE;
}

}

#ifdef SMYD_SPLASH_SCREEN_UNIT_TEST

namespace
{

double progress = 0.;

gboolean addProgress(gpointer splash)
{
    char buffer[100];
    Shell::SplashScreen *s = static_cast<Shell::SplashScreen *>(splash);
    if (progress >= 0.)
    {
        gtk_main_quit();
        return FALSE;
    }
    progress += 0.2;
    snprintf(buffer, sizeof(buffer),
             "Testing splash screen.  Finished %.2f.", progress);
    s->setProgress(progress, buffer);
    return TRUE;
}

}

int main(int argc, char *argv[])
{
    char buffer[100];
    gtk_init(&argc, &argv);
    Shell::SplashScreen splash("Splash screen test",
                               "../../../images/splash-screen.png");
    snprintf(buffer, sizeof(buffer),
             "Testing splash screen.  Finished %.2f.", progress);
    splash.setProgress(progress, buffer);
    g_timeout_add(2000, addProgress, &splash);
    gtk_main();
    return 0;
}

#endif
