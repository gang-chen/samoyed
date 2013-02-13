// Bar.
// Copyright (C) 2013 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "bar.hpp"
#include "widget-with-bars.hpp"

namespace Samoyed
{

Bar::~Bar()
{
    assert(m_parent);
    m_parent->onBarClosed(this);
}

}
