// Change hint.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_CHANGE_HINT_HPP
#define SMYD_CHANGE_HINT_HPP

#include "revision.hpp"
#include "range.hpp"

namespace Samoyed
{

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
