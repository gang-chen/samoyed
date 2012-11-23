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

    SplitPane *split(Type type);

    ~Pane();

    Pane *leftNeighbor() const;
    Pane *rightNeighbor() const;
    Pane *upperNeighbor() const;
    Pane *lowerNeighbor() const;

private:
    Pane();

    SplitPane *m_parent;
};

}

#endif
