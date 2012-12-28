// Bar.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_BAR_HPP
#define SMYD_BAR_HPP

#include <boost/utility.hpp>
#include <gtk/gtk.h>

namespace Samoyed
{

class Bar: public boost::noncopyable
{
public:
    enum Orientation
    {
        ORIENTATION_HORIZONTAL = GTK_ORIENTATION_HORIZONTAL,
        ORIENTATION_VERTICAL = GTK_ORIENTATION_VERTICAL
    };

    virtual ~Bar() {}

    virtual Orientation orientation() = 0;

    virtual GtkWidget *gtkWidget() = 0;
}

#endif
