// Pane base.
// Copyright (C) 2012 Gang Chen.

#include "pane-base.hpp"
#include "window.hpp"
#include "split-pane.hpp"
#include <assert.h>

namespae Samoyed
{

PaneBase::~PaneBase()
{
    assert((window() && !parent()) || (!window() && parent()));
    if (window())
        window()->onContentClosed();
    else if (parent())
        parent()->onChildClosed(this);
}

}
