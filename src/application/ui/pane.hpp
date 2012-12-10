// Pane.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_PANE_HPP
#define SMYD_PANE_HPP

#include "pane-base.hpp"

namespace Samoyed
{

class SplitPane;

class Pane: public PaneBase
{
public:
    Pane(Type type): PaneBase(type) {}
    virtual ~Pane();

    SplitPane *split(Type type);

    virtual Pane &currentPane() { return *this; }
    virtual const Pane &currentPane() const { return *this; }
};

}

#endif
