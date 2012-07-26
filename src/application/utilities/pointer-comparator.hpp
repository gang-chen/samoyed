// Pointer comparator.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_POINTER_COMPARATOR_HPP
#define SMYD_POINTER_COMPARATOR_HPP

namespace Samoyed
{

template<class T> struct PointerComparator
{
    bool operator()(const T *lhs, const T *rhs) const
    { return *lhs < *rhs; }
};

}

#endif
