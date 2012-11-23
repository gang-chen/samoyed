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
    enum Type
    {
        TYPE_EDITOR_GROUP,
        TYPE_PROJECT_EXPLORER
    };

    static Pane *create(Type type);

    Pane(Type type): m_type(type) {}
    virtual ~Pane();

    SplitPane *split(Type type);

    virtual Pane *currentPane() { return this; }
    virtual const Pane *currentPane() const { return this; }

    Pane *leftNeighbor() const;
    Pane *rightNeighbor() const;
    Pane *upperNeighbor() const;
    Pane *lowerNeighbor() const;

private:
    Type m_type;
};

}

#endif
