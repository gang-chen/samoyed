// Splash screen.
// Copyright (C) 2011 Gang Chen.

/*
UNIT TEST BUILD
g++ splash-screen.cpp -DSMYD_SPLASH_SCREEN_UNIT_TEST\
 `pkg-config --cflags --libs gtk+-3.0` -Werror -Wall -o splash-screen
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "splash-screen.hpp"
#include <gtk/gtk.h>
#ifdef SMYD_SPLASH_SCREEN_UNIT_TEST
# include <stdio.h>
#endif

namespace Samoyed
{

SplashScreen::SplashScreen(const char *title, const char *imageFileName)
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkImage *image = gtk_image_new_from_file(imageFileName);
    m_progressBar = gtk_progress_bar_new();
    gtk_window_set_decorated(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_position(GTK_WINDOW(m_window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_title(GTK_WINDOW(m_window), title);
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(m_progressBar), TRUE);
    gtk_container_add(GTK_CONTAINER(m_window), box);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), m_progressBar, FALSE, FALSE, 0);
    g_signal_connect(m_window,
                     "delete-event",
                     G_CALLBACK(onDeleteEvent),
                     this);
    gtk_widget_show_all(m_window);
}

SplashScreen::~SplashScreen()
{
    gtk_widget_destroy(m_window);
}

void SplashScreen::setProgress(double progress, const char *message)
{
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(m_progressBar), progress);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(m_progressBar), message);
}

gboolean SplashScreen::onDeleteEvent(GtkWidget *widget,
                                     GdkEvent *event,
                                     gpointer window)
{
    // Disable closing the splash screen.
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
    Samoyed::SplashScreen *s = static_cast<Samoyed::SplashScreen *>(splash);
    if (progress >= 1.)
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
    Samoyed::SplashScreen splash("Splash screen test",
                                 "../../../data/splash-screen.png");
    snprintf(buffer, sizeof(buffer),
             "Testing splash screen. Finished %.2f.", progress);
    splash.setProgress(progress, buffer);
    g_timeout_add(2000, addProgress, &splash);
    gtk_main();
    return 0;
}

#endif
