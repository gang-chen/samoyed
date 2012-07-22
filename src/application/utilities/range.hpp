// Range.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_RANGE_HPP
#define SMYD_RANGE_HPP

#include <algorithm>

namespace Samoyed
{

/**
 * A range represents the range of the impacted characters after a source file
 * is changed.
 *
 * Usually, a range records the byte index of the inclusive first impacted
 * character and the byte index of the exclusive last impacted character.  There
 * are two exceptions.  One is to represent the impacted range after some
 * characters are removed.  Actually, no character in the changed file was
 * impacted.  In this case, the range records where the change happened.  The
 * other exception is to represent the range for a null change.  We use a
 * special empty range to represent it.
 */
class Range
{
public:
	/**
	 * Construct an empty range.
	 */
	Range(): m_begin(0), m_end(-1) {}

	/**
	 * @param begin The inclusive first character.
	 * @paran end The exclusive last character.
	 */
	Range(int begin, int end): m_begin(begin), m_end(end)
	{}

	int begin() const { return m_begin; }

	int end() const { return m_end; }

	void setBegin(int begin) { m_begin = begin; }

	void setEnd(int end) { m_end = end; }

	bool empty() const { return m_begin > m_end; }

	bool operator==(const Range &rhs) const
	{
		return m_begin == rhs.m_begin && m_end == rhs.m_end;
	}

	bool operator!=(const Range &rhs) const
	{
		return !(*this == rhs);
	}

	Range operator+(const Range &rhs) const
	{
		if (empty())
			return rhs;
		if (rhs.empty())
			return *this;
		return Range(std::min(m_begin, rhs.m_begin),
					 std::max(m_end, rhs.m_end));
	}

	const Range &operator+=(const Range &rhs)
	{
		if (empty())
			*this = rhs;
		else if (!rhs.empty())
		{
			m_begin = std::min(m_begin, rhs.m_begin);
			m_end = std::max(m_end, rhs.m_end);
		}
		return *this;
	}

private:
	int m_begin;
	int m_end;
};

}

#endif
