// String comparator.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_STRING_COMPARATOR_HPP
#define SMYD_STRING_COMPARATOR_HPP

#include <string.h>

namespace Samoyed
{

struct StringComparator
{
    bool operator()(const char *lhs, const char *rhs) const
    { return strcmp(lhs, rhs) < 0; }
};

}

#endif
