// Buffer of text.
// Copyright (C) 2010 Gang Chen.

#ifndef SMYD_TEXT_BUFFER_HPP
#define SMYD_TEXT_BUFFER_HPP

#include "buffer.hpp"
#include "utf8.hpp"

namespace Samoyed
{

struct Utf8CharTraits
{
    static int length(const char *atom)
    {
        return Utf8::length(atom);
    }

    static int previousAtomLength(const char *atom)
    {
        return atom - Utf8::begin(atom - 1);
    }

    static int length3(const char *atom)
    {
        if (*atom == '\n')
            return 1;
        return 0;
    }

    static int skip3(const char *)
    {
        return 0;
    }
};

const int TEXT_BUFFER_BLOCK_SIZE = 20000;

/**
 * A text buffer stores a stream of UTF-8 encoded characters.
 *
 * A text buffer uses line offsets as the tertiary indexes.
 */
class TextBuffer:
    public Buffer<char, Utf8CharTraits, TEXT_BUFFER_BLOCK_SIZE, -1, true, true>
{
public:
    int byteCount() const { return length(); }

    int characterCount() const { return length2(); }

    int lineCount() const { return length3() + 1; }

    void setLineColumn(int line, int column);

    void transformByteOffsetToLineColumn(int byteOffset,
                                         int &line,
                                         int &column) const;

    void transformLineColumnToByteOffset(int line,
                                         int column,
                                         int &byteOffset) const;

    void transformCharOffsetToLineColumn(int charOffset,
                                         int &line,
                                         int &column) const;

    void transformLineColumnToCharOffset(int line,
                                         int column,
                                         int &charOffset) const;
};

template<class Iterator> void getIteratorLineColumn(const Iterator &it,
                                                    int &line,
                                                    int &column)
{
    Iterator it2(it);
    line = it2.index3();
    it2.set3(line);
    column = it.index2() - it2.index2();
}

template<class Iterator> void setIteratorLineColumn(Iterator &it,
                                                    int line,
                                                    int column)
{
    it.set3(line);
    it.move2(column);
}

}

#endif
