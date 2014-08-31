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
#include <glib.h>
#include <gtk/gtk.h>

namespace Samoyed
{

SplashScreen::SplashScreen(const char *title, const char *imageFileName)
{
    m_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *grid = gtk_grid_new();
    GtkWidget *image = gtk_image_new_from_file(imageFileName);
    m_progressBar = gtk_progress_bar_new();
    gtk_window_set_decorated(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_position(GTK_WINDOW(m_window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(m_window), FALSE);
    gtk_window_set_title(GTK_WINDOW(m_window), title);
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(m_progressBar), TRUE);
    gtk_container_add(GTK_CONTAINER(m_window), grid);
    gtk_grid_attach(GTK_GRID(grid), image, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), m_progressBar, 0, 1, 1, 1);
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

}

#ifdef SMYD_SPLASH_SCREEN_UNIT_TEST

double progress = 0.;

gboolean onDeleteEvent(GtkWidget *widget, GdkEvent *event,
                       Samoyed::SplashScreen *splash)
{
    delete splash;
    gtk_main_quit();
    return TRUE;
}

gboolean addProgress(gpointer splash)
{
    Samoyed::SplashScreen *s = static_cast<Samoyed::SplashScreen *>(splash);
    if (progress >= 1.)
    {
        delete s;
        gtk_main_quit();
        return FALSE;
    }
    progress += 0.2;
    char *desc = g_strdup_printf("Testing splash screen.  Finished %.2f.",
                                 progress);
    s->setProgress(progress, desc);
    g_free(desc);
    return TRUE;
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);
    Samoyed::SplashScreen *splash =
        new Samoyed::SplashScreen("Splash screen test",
                                  "../../../data/splash-screen.png");
    char *msg = g_strdup_printf("Testing splash screen. Finished %.2f.",
                                progress);
    splash->setProgress(progress, msg);
    g_free(msg);
    g_signal_connect(splash->gtkWidget(), "delete-event",
                     G_CALLBACK(onDeleteEvent), splash);
    g_timeout_add(2000, addProgress, splash);
    gtk_main();
    return 0;
}

#endif
