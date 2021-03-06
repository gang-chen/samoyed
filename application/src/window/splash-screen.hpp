// Splash screen.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_SPLASH_SCREEN_HPP
#define SMYD_SPLASH_SCREEN_HPP

#include <boost/utility.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class SplashScreen: public boost::noncopyable
{
public:
    SplashScreen(const char *title, const char *imageFileName);
    ~SplashScreen();
    void setProgress(double progress, const char *message);
    GtkWidget *gtkWidget() const { return m_window; }

private:
    GtkWidget *m_window;
    GtkWidget *m_progressBar;
};

}

#endif
