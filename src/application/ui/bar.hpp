// Bar.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_BAR_HPP
#define SMYD_BAR_HPP

#include "widget.hpp"
#include "../utilities/miscellaneous.hpp"

namespace Samoyed
{

/**
 * A bar is an orientable widget that can only be contained by a widget with
 * bars.
 */
class Bar: public Widget, public Orientable
{
};

}

#endif
