// Buffer.
// Copyright (C) 2011 Gang Chen.

/*
g++ buffer.cpp -DSMYD_BUFFER_UNIT_TEST -Wall -o buffer
*/

#ifdef SMYD_BUFFER_UNIT_TEST

#include "buffer.hpp"
#include <assert.h>
#include <string>

template<class Unit> struct SimpleAtomTraits
{
    static int length(const Unit *atom) { return 1; }
    static int previousAtomLength(const Unit *atom) { return 1; }
    static int length3(const Unit *atom) { return 3; }
    static int skip3(const Unit *atom) { return 2; }
};

typedef Samoyed::Buffer<char, SimpleAtomTraits<char>, 10, -1, true, true>
    SimpleBuffer;

void testEquality(const SimpleBuffer &buffer,
                  const std::string &string)
{
    assert(buffer.length() == static_cast<int>(string.length()));
    char *t = new char[buffer.length() + 1];
    buffer.getAtoms(0, buffer.length(), t);
    t[buffer.length()] = '\0';
    assert(string == t);
    delete[] t;
}

void testInsertion(SimpleBuffer &buffer,
                   SimpleBuffer &buffer2,
                   std::string &string,
                   const char *text,
                   int length)
{
    int oldIndex = buffer.cursor();
    buffer.insert(text, length, -1, -1);
    int len = length;
    const char *cp = text;
    while (len)
    {
        int l = len;
        char *gap = buffer2.makeGap(l);
        if (l > len)
            l = len;
        memcpy(gap, cp, sizeof(char) * l);
        buffer2.insertInGap(gap, l, -1, -1);
        len -= l;
        cp += l;
    }
    string.insert(oldIndex, text, length);
    int newIndex = buffer.cursor();
    assert(oldIndex + length == newIndex);
    testEquality(buffer, string);
    testEquality(buffer2, string);
}

void testRemoval(SimpleBuffer &buffer,
                 SimpleBuffer &buffer2,
                 std::string &string,
                 int length)
{
    int oldIndex = buffer.cursor();
    buffer.remove(length, -1, -1);
    buffer2.remove(length, -1, -1);
    string.erase(oldIndex, length);
    int newIndex = buffer.cursor();
    assert(oldIndex == newIndex);
    testEquality(buffer, string);
    testEquality(buffer2, string);
}

int main()
{
    const char *TEXT = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    SimpleBuffer buffer, buffer2;
    std::string string;
    testEquality(buffer, string);
    testEquality(buffer2, string);
    // Insert at the end.
    testInsertion(buffer, buffer2, string, TEXT, 0);
    testInsertion(buffer, buffer2, string, TEXT, 3);
    testInsertion(buffer, buffer2, string, TEXT, 7);
    testInsertion(buffer, buffer2, string, TEXT, 30);
    // Insert at the beginning of a block.
    buffer.setCursor(10);
    buffer2.setCursor2(10);
    testInsertion(buffer, buffer2, string, TEXT, 1);
    testInsertion(buffer, buffer2, string, TEXT, 12);
    testInsertion(buffer, buffer2, string, TEXT, 12);
    // Insert at the middle of a block.
    buffer.setCursor(15);
    buffer2.setCursor3(buffer.cursor3());
    testInsertion(buffer, buffer2, string, TEXT, 1);
    testInsertion(buffer, buffer2, string, TEXT, 9);
    buffer.setCursor(15);
    buffer2.setCursor3(buffer.cursor3());
    testInsertion(buffer, buffer2, string, TEXT, 10);
    testInsertion(buffer, buffer2, string, TEXT, 3);
    // Insert by moving the gap.
    buffer.moveCursor(-3);
    buffer2.moveCursor2(-3);
    testInsertion(buffer, buffer2, string, TEXT, 1);
    testInsertion(buffer, buffer2, string, TEXT, 9);
    buffer.moveCursor(-3);
    buffer2.moveCursor2(-3);
    for (int i = 0; i < 15; ++i)
        testInsertion(buffer, buffer2, string, TEXT, 1);
    // Insert at the beginning.
    buffer.setCursor(0);
    buffer2.setCursor(0);
    testInsertion(buffer, buffer2, string, TEXT, 1);
    testInsertion(buffer, buffer2, string, TEXT, 12);
    // Insert in a pseudo-random way.
    for (int i = 0; i < 100; ++i)
    {
        buffer.setCursor(buffer.length() / 2);
        buffer2.setCursor(buffer.cursor());
        testInsertion(buffer, buffer2, string, TEXT, 1);
        testInsertion(buffer, buffer2, string, TEXT, buffer.length() % 17);
        testInsertion(buffer, buffer2, string, TEXT, 1);
    }
    // Test constant iterators.
    SimpleBuffer::ConstIterator i1(buffer, 0, -1, -1);
    SimpleBuffer::ConstIterator i2(buffer, 0, -1, -1);
    assert(i1 == i2);
    i1.move(10);
    assert(i1 != i2);
    i2.move(10);
    assert(i1 == i2);
    i1.move3(3);
    assert(i1 != i2);
    i2.move3(3);
    assert(i1 == i2);
    // Remove from the end.
    buffer.setCursor(buffer.length());
    buffer2.setCursor3(buffer.cursor3());
    buffer.moveCursor(-3);
    buffer2.setCursor3(buffer.cursor3());
    testRemoval(buffer, buffer2, string, 3);
    buffer.moveCursor(-10);
    buffer2.setCursor3(buffer.cursor3());
    testRemoval(buffer, buffer2, string, 10);
    // Remove from the beginning.
    buffer.setCursor(0);
    buffer2.setCursor3(buffer.cursor3());
    buffer2.moveCursor2(5);
    buffer2.setCursor2(0);
    testRemoval(buffer, buffer2, string, 8);
    // Remove and insert in a pseudo-ramdon way.
    while (buffer.length())
    {
        if (buffer.length() >= 17)
        {
            buffer.setCursor(buffer.length() / 2);
            buffer2.setCursor2(buffer.cursor2());
            testInsertion(buffer, buffer2, string, TEXT, 3);
            testRemoval(buffer, buffer2, string, 7);
            testInsertion(buffer, buffer2, string, TEXT, 3);
        }
        buffer.setCursor(buffer.length() / 2);
        buffer2.setCursor3(buffer.cursor3());
        testRemoval(buffer, buffer2, string, 1);
        if (buffer.length() - buffer.cursor() >= 17)
        {
            testRemoval(buffer, buffer2, string, buffer.length() % 17);
            testRemoval(buffer, buffer2, string, 1);
        }
    }
    return 0;
}

#endif // #ifdef SMYD_BUFFER_UNIT_TEST
