// Pane base.
// Copyright (C) 2012 Gang Chen.

#include "pane-base.hpp"
#include "window.hpp"
#include "split-pane.hpp"

namespae Samoyed
{

PaneBase::~PaneBase()
{
    if (window())
        window()->onPaneClosed();
    else if (parent())
        parent()->onChildClosed(*this);
}

}
