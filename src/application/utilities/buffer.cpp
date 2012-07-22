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
				   std::string &string,
				   const char *text,
				   int length)
{
	int oldIndex = buffer.cursor();
	buffer.insert(text, length, -1, -1);
	string.insert(oldIndex, text, length);
	int newIndex = buffer.cursor();
	assert(oldIndex + length == newIndex);
	testEquality(buffer, string);
}

void testRemoval(SimpleBuffer &buffer,
				 std::string &string,
				 int length)
{
	int oldIndex = buffer.cursor();
	buffer.remove(length, -1, -1);
	string.erase(oldIndex, length);
	int newIndex = buffer.cursor();
	assert(oldIndex == newIndex);
	testEquality(buffer, string);
}

int main()
{
	const char *TEXT = "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	SimpleBuffer buffer;
	std::string string;
	testEquality(buffer, string);
	// Insert at the end.
	testInsertion(buffer, string, TEXT, 0);
	testInsertion(buffer, string, TEXT, 3);
	testInsertion(buffer, string, TEXT, 7);
	testInsertion(buffer, string, TEXT, 30);
	// Insert at the beginning of a block.
	buffer.setCursor(10);
	testInsertion(buffer, string, TEXT, 1);
	testInsertion(buffer, string, TEXT, 12);
	testInsertion(buffer, string, TEXT, 12);
	// Insert at the middle of a block.
	buffer.setCursor(15);
	testInsertion(buffer, string, TEXT, 1);
	testInsertion(buffer, string, TEXT, 9);
	buffer.setCursor(15);
	testInsertion(buffer, string, TEXT, 10);
	testInsertion(buffer, string, TEXT, 3);
	// Insert by moving the gap.
	buffer.moveCursor(-3);
	testInsertion(buffer, string, TEXT, 1);
	testInsertion(buffer, string, TEXT, 9);
	buffer.moveCursor(-3);
	for (int i = 0; i < 15; ++i)
		testInsertion(buffer, string, TEXT, 1);
	// Insert at the beginning.
	buffer.setCursor(0);
	testInsertion(buffer, string, TEXT, 1);
	testInsertion(buffer, string, TEXT, 12);
	// Insert in a pseudo-random way.
	for (int i = 0; i < 100; ++i)
	{
		buffer.setCursor(buffer.length() / 2);
		testInsertion(buffer, string, TEXT, 1);
		testInsertion(buffer, string, TEXT, buffer.length() % 17);
		testInsertion(buffer, string, TEXT, 1);
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
	buffer.moveCursor(-3);
	testRemoval(buffer, string, 3);
	buffer.moveCursor(-10);
	testRemoval(buffer, string, 10);
	// Remove from the beginning.
	buffer.setCursor(0);
	testRemoval(buffer, string, 8);
	// Remove in a pseudo-ramdon way.
	while (buffer.length())
	{
		buffer.setCursor(buffer.length() / 2);
		testRemoval(buffer, string, 1);
		if (buffer.length() - buffer.cursor() >= 17)
		{
			testRemoval(buffer, string, buffer.length() % 17);
			testRemoval(buffer, string, 1);
		}
	}
	return 0;
}

#endif // #ifdef SMYD_BUFFER_UNIT_TEST
