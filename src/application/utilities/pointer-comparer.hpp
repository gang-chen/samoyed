// Pointer comparer.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_POINTER_COMPARER_HPP
#define SMYD_POINTER_COMPARER_HPP

namespace Samoyed
{

template<class T> struct PointerComparer
{
    bool operator()(const T *lhs, const T *rhs) const
    { return *lhs < *rhs; }
};

}

#endif
