// Pane.
// Copyright (C) 2012 Gang Chen.

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "pane.hpp"
#include "window.hpp"
#include "split-pane.hpp"
#include <assert.h>

namespace Samoyed
{

Pane::~Pane()
{
    assert((window() && !parent()) || (!window() && parent()));
    if (window())
        window()->onContentClosed();
    else if (parent())
        parent()->onChildClosed(this);
}

}
