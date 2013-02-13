// Bar.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_BAR_HPP
#define SMYD_BAR_HPP

#include "widget.hpp"

namespace Samoyed
{

class WidgetWithBars;

class Bar: public Widget
{
public:
    enum Orientation
    {
        ORIENTATION_HORIZONTAL = GTK_ORIENTATION_HORIZONTAL,
        ORIENTATION_VERTICAL = GTK_ORIENTATION_VERTICAL
    };

    virtual Orientation orientation() const = 0;

private:
    WidgetWithBars *m_parent;
};

}

#endif
