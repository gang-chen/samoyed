// Bar.
// Copyright (C) 2013 Gang Chen.

#ifndef HAVE_CONFIG_H
# include <config.h>
#endif
#include "bar.hpp"
#include "widget-with-bars.hpp"

namespace Samoyed
{

Bar::~Bar()
{
    if (parent())
        static_cast<WidgetWithBars *>(parent())->removeBar(*this);
}

}
