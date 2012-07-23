// UTF-8 encoding.
// Copyright (C) 2011 Gang Chen.

/*
g++ utf8.cpp -DSMYD_UTF8_UNIT_TEST -Wall -o utf8
*/

#include "utf8.hpp"

#ifdef SMYD_UTF8_UNIT_TEST
# include <assert.h>
#endif

namespace Samoyed
{

const int Utf8::LENGTH_TABLE[256] =
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,0,0
};

bool Utf8::validate(const char *cp,
                    int length,
                    const char *&validCp)
{
    const char *end = cp + length;
    validCp = cp;
    while (cp != end && *cp)
    {
        int l = LENGTH_TABLE[static_cast<unsigned char>(*cp)];
        if (l == 0)
            return false;
        const char *e = cp + l;
        for (++cp; cp < e; ++cp)
        {
            if (cp == end || !*cp)
                return true;
            if ((*cp & 0xc0) != 0x80)
                return false;
        }
        validCp = cp;
    }
    return true;
}

}

#ifdef SMYD_UTF8_UNIT_TEST

int main()
{
    const char *t;
    const char *v;

    t = "\x01";
    assert(Samoyed::Utf8::length(t) == 1);
    assert(Samoyed::Utf8::begin(t) == t);
    assert(Samoyed::Utf8::validate(t, -1, v));
    assert(v == t + 1);
    assert(Samoyed::Utf8::validate(t, 1, v));
    assert(v == t + 1);

    t = "\xc1\x81";
    assert(Samoyed::Utf8::length(t) == 2);
    assert(Samoyed::Utf8::begin(t) == t);
    assert(Samoyed::Utf8::begin(t + 1) == t);
    assert(Samoyed::Utf8::validate(t, -1, v));
    assert(v == t + 2);
    assert(Samoyed::Utf8::validate(t, 1, v));
    assert(v == t);
    assert(Samoyed::Utf8::validate(t, 2, v));
    assert(v == t + 2);

    t = "\xf1\x81\x82\x83";
    assert(Samoyed::Utf8::length(t) == 4);
    assert(Samoyed::Utf8::begin(t) == t);
    assert(Samoyed::Utf8::begin(t + 1) == t);
    assert(Samoyed::Utf8::begin(t + 2) == t);
    assert(Samoyed::Utf8::begin(t + 3) == t);
    assert(Samoyed::Utf8::validate(t, -1, v));
    assert(v == t + 4);
    assert(Samoyed::Utf8::validate(t, 1, v));
    assert(v == t);
    assert(Samoyed::Utf8::validate(t, 2, v));
    assert(v == t);
    assert(Samoyed::Utf8::validate(t, 3, v));
    assert(v == t);
    assert(Samoyed::Utf8::validate(t, 4, v));
    assert(v == t + 4);

    t = "\xf1\x81\xc1\x81";
    assert(Samoyed::Utf8::length(t) == 4);
    assert(Samoyed::Utf8::begin(t) == t);
    assert(Samoyed::Utf8::begin(t + 1) == t);
    assert(!Samoyed::Utf8::validate(t, -1, v));
    assert(v == t);
    assert(Samoyed::Utf8::validate(t, 1, v));
    assert(v == t);
    assert(Samoyed::Utf8::validate(t, 2, v));
    assert(v == t);
    assert(!Samoyed::Utf8::validate(t, 3, v));
    assert(v == t);
    assert(!Samoyed::Utf8::validate(t, 4, v));
    assert(v == t);

    t = "\xc1\x81\xf1\x81\xc1\x81";
    assert(Samoyed::Utf8::length(t) == 2);
    assert(Samoyed::Utf8::length(t + 2) == 4);
    assert(Samoyed::Utf8::begin(t) == t);
    assert(Samoyed::Utf8::begin(t + 1) == t);
    assert(Samoyed::Utf8::begin(t + 2) == t + 2);
    assert(Samoyed::Utf8::begin(t + 3) == t + 2);
    assert(!Samoyed::Utf8::validate(t, -1, v));
    assert(v == t + 2);
    assert(Samoyed::Utf8::validate(t, 1, v));
    assert(v == t);
    assert(Samoyed::Utf8::validate(t, 2, v));
    assert(v == t + 2);
    assert(Samoyed::Utf8::validate(t, 3, v));
    assert(v == t + 2);
    assert(Samoyed::Utf8::validate(t, 4, v));
    assert(v == t + 2);
    assert(!Samoyed::Utf8::validate(t, 5, v));
    assert(v == t + 2);
    assert(!Samoyed::Utf8::validate(t, 6, v));
    assert(v == t + 2);

    return 0;
}

#endif // #ifdef SMYD_UTF8_UNIT_TEST
