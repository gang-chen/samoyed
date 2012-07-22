// Range.
// Copyright (C) 2011 Gang Chen.

/*
g++ range.cpp -DSMYD_RANGE_UNIT_TEST -Wall -o range
*/

#ifdef SMYD_RANGE_UNIT_TEST

#include "range.hpp"
#include <assert.h>

int main()
{
	Samoyed::Range r1;
	Samoyed::Range r2(0, 0);
	Samoyed::Range r3(1, 3);
	Samoyed::Range r4(2, 2);
	Samoyed::Range r5(0, 3);
	Samoyed::Range r6(4, 5);
	Samoyed::Range r7(0, 5);
	assert(r1.empty());
	assert(!r2.empty());
	assert(!r3.empty());
	assert(r1 != r2);
	assert(r1 + r2 == r2);
	assert((r1 += r3) == r3);
	assert(r2 + r3 == r5);
	assert(r3 + r4 == r3);
	assert(r2 + r3 + r6 == r7);
	return 0;
}

#endif // #ifdef SMYD_RANGE_UNIT_TEST
