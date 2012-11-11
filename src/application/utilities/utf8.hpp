// UTF-8 encoding.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_UTF8_HPP_INCLUDED
#define SMYD_UTF8_HPP_INCLUDED

#include <assert.h>

namespace Samoyed
{

/**
 * A collection of utility functions for UTF-8 encoded strings.
 */
class Utf8
{
public:
    /**
     * Get the length of a UTF-8 encoded character.
     * @param cp The beginning of a UTF-8 encoded character.
     * @return The number of the bytes composing the character.
     */
    static inline int length(const char *cp);

    /**
     * Get the beginning of a UTF-8 encoded character.
     * @param cp A pointer to any byte of a UTF-8 encoded character.
     * @return The beginning of the character.
     */ 
    static inline const char* begin(const char *cp);

    /**
     * Validate a stream of UTF-8 encoded characters.
     * @param cp A stream of UTF-8 encoded characters.
     * @param length The number of the bytes to be validated, or -1 to validate
     * until a '\0'.
     * @param validCp Return the end of the valid complete characters.
     * @return True iff all the characters are valid even though the last
     * character may be incomplete.
     */
    static bool validate(const char *cp,
                         int length,
                         const char *&validCp);

    /**
     * Count the number of UTF-8 encoded characters.
     * @param cp A stream of UTF-8 encoded characters.
     * @param length The number of the bytes to be validated, or -1 to validate
     * until a '\0'.
     */
    static inline int countCharacters(const char *cp, int length);

private:
    static const int LENGTH_TABLE[256];
};

inline int Utf8::length(const char *cp)
{
    assert((*cp & 0xc0) != 0x80);
    assert(static_cast<unsigned char>(*cp) != 0xfe);
    assert(static_cast<unsigned char>(*cp) != 0xff);
    return LENGTH_TABLE[static_cast<unsigned char>(*cp)];
}

inline const char* Utf8::begin(const char* cp)
{
    while ((*cp & 0xc0) == 0x80)
        --cp;
    return cp;
}

inline int Utf8::countCharacters(const char *cp, int length)
{
    const char *end = cp + length;
    int n = 0;
    while (cp != end && *cp)
    {
        cp += Utf8::length(cp);
        ++n;
    }
    return n;
}

}

#endif
