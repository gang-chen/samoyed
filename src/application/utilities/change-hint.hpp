// Change hint.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_CHANGE_HINT_HPP
#define SMYD_CHANGE_HINT_HPP

#include "revision.hpp"
#include "range.hpp"

namespace Samoyed
{

/**
 * There are two special types of changes.  One type is dummy changes that do
 * not change the contents but only change the revisions, typically due to
 * writing to external files.  The other type of changes keep the revisions
 * unchanged but change the contents, typically due to rereading external files
 * after reading failures.
 */
class ChangeHint
{
public:
    ChangeHint(const Revision &oldRevision,
               const Revision &newRevision,
               const Range &range):
        m_oldRevision(oldRevision),
        m_newRevision(newRevision),
        m_range(range)
    {}

    const Revision &oldRevision() const { return m_oldRevision; }

    const Revision &newRevision() const { return m_newRevision; }

    const Range &range() const { return m_range; }

    bool zero() const { return m_range.empty(); }

private:
    Revision m_oldRevision;

    Revision m_newRevision;

    Range m_range;
};

}

#endif
