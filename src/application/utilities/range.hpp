// Range.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_RANGE_HPP
#define SMYD_RANGE_HPP

#include <algorithm>

namespace Samoyed
{

/**
 * A range represents the range of the characters impacted by a change to a
 * source file.
 *
 * Typically, a range records the byte offset of the inclusive first impacted
 * character and the byte offset of the exclusive last impacted character.
 * There are two exceptions.  One is to represent the range impacted by a
 * removal of some characters.  Actually, no character in the changed file was
 * impacted.  In this case, the range records the byte offset of the first
 * removed character, telling where the removal happened.  Such a range is
 * called a single-point range.  The other exception is to represent the range
 * for a null change.  We use a special empty range to represent it.
 */
class Range
{
public:
    /**
     * Construct an empty range.
     */
    Range(): m_begin(0), m_end(-1) {}

    /**
     * Construct a single-point range.
     * @param point The byte offset of the point.
     */
    Range(int point): m_begin(point), m_end(point) {}

    /**
     * @param begin The byte offset of the inclusive first character.
     * @paran end The byte offset of the exclusive last character.
     */
    Range(int begin, int end): m_begin(begin), m_end(end)
    {}

    bool empty() const { return m_begin > m_end; }

    int point() const { return m_begin == m_end ? m_begin : -1; }

    int begin() const { return m_begin; }

    int end() const { return m_end; }

    void setEmpty() { m_begin = 0; m_end = -1; }

    void setPoint(int point) { m_begin = m_end = point; }

    void setBegin(int begin) { m_begin = begin; }

    void setEnd(int end) { m_end = end; }

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
