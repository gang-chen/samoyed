// Bar.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_BAR_HPP
#define SMYD_BAR_HPP

#include "widget.hpp"

namespace Samoyed
{

/**
 * A bar is a widget that can only be contained by a widget with bars.
 */
class Bar: public Widget
{
public:
    enum Orientation
    {
        ORIENTATION_HORIZONTAL = GTK_ORIENTATION_HORIZONTAL,
        ORIENTATION_VERTICAL = GTK_ORIENTATION_VERTICAL
    };

    virtual Orientation orientation() const = 0;
};

}

#endif
