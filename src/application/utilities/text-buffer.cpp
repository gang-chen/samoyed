// Text buffer.
// Copyright (C) 2011 Gang Chen.

/*
g++ text-buffer.cpp utf8.cpp -DSMYD_TEXT_BUFFER_UNIT_TEST -Wall -o text-buffer
*/

#ifdef SMYD_TEXT_BUFFER_UNIT_TEST

#include "text-buffer.hpp"
#include <assert.h>
#include <string.h>

int main()
{
	const char *TEXT1 = "hello world\n";
	const char *TEXT2 = "\xe4\xbd\xa0\xe5\xa5\xbd\n";

	Samoyed::TextBuffer buffer;
	char text[100];

	assert(buffer.length() == 0);
	assert(buffer.length2() == 0);
	assert(buffer.length3() == 0);

	buffer.insert(TEXT1, strlen(TEXT1), -1, -1);
	assert(buffer.cursor() == buffer.length());
	assert(buffer.cursor2() == buffer.length2());
	assert(buffer.cursor3() == buffer.length3());
	assert(buffer.length2() == buffer.length());
	assert(buffer.length3() == 1);

	buffer.insert(TEXT1, strlen(TEXT1), -1, -1);
	assert(buffer.cursor() == buffer.length());
	assert(buffer.cursor2() == buffer.length2());
	assert(buffer.cursor3() == buffer.length3());
	assert(buffer.length2() == buffer.length());
	assert(buffer.length3() == 2);

	buffer.getAtoms(0, strlen(TEXT1), text);
	assert(memcmp(TEXT1, text, strlen(TEXT1)) == 0);

	buffer.getAtoms(strlen(TEXT1), strlen(TEXT1), text);
	assert(memcmp(TEXT1, text, strlen(TEXT1)) == 0);

	buffer.setCursor(1);
	assert(buffer.cursor() == 1);
	assert(buffer.cursor2() == 1);
	assert(buffer.cursor3() == 0);

	buffer.setCursor2(3);
	assert(buffer.cursor() == 3);
	assert(buffer.cursor2() == 3);
	assert(buffer.cursor3() == 0);

	buffer.setCursor2(15);
	assert(buffer.cursor() == 15);
	assert(buffer.cursor2() == 15);
	assert(buffer.cursor3() == 1);

	buffer.setCursor3(1);
	assert(static_cast<unsigned>(buffer.cursor()) == strlen(TEXT1));
	assert(static_cast<unsigned>(buffer.cursor2()) == strlen(TEXT1));
	assert(buffer.cursor3() == 1);

	buffer.setCursor3(0);
	assert(buffer.cursor() == 0);
	assert(buffer.cursor2() == 0);
	assert(buffer.cursor3() == 0);

	buffer.setCursor3(2);
	assert(buffer.cursor() == buffer.length());
	assert(buffer.cursor2() == buffer.length2());
	assert(buffer.cursor3() == buffer.length3());

	buffer.moveCursor(-1);
	buffer.remove(1, -1, -1);
	assert(static_cast<unsigned>(buffer.length()) == strlen(TEXT1) * 2 - 1);
	assert(static_cast<unsigned>(buffer.length2()) == strlen(TEXT1) * 2 - 1);
	assert(buffer.length3() == 1);

	buffer.setCursor3(1);
	assert(static_cast<unsigned>(buffer.cursor()) == strlen(TEXT1));
	assert(static_cast<unsigned>(buffer.cursor2()) == strlen(TEXT1));
	assert(buffer.cursor3() == 1);

	buffer.setCursor3(0);
	assert(buffer.cursor() == 0);
	assert(buffer.cursor2() == 0);
	assert(buffer.cursor3() == 0);

	buffer.removeAll();
	assert(buffer.cursor() == buffer.length());
	assert(buffer.cursor2() == buffer.length2());
	assert(buffer.cursor3() == buffer.length3());
	assert(buffer.length() == 0);
	assert(buffer.length2() == 0);
	assert(buffer.length3() == 0);

	buffer.insert(TEXT1, strlen(TEXT1), -1, -1);
	buffer.insert(TEXT2, strlen(TEXT2), -1, -1);
	assert(buffer.cursor() == buffer.length());
	assert(buffer.cursor2() == buffer.length2());
	assert(buffer.cursor3() == buffer.length3());
	assert(static_cast<unsigned>(buffer.length()) ==
		   strlen(TEXT1) + strlen(TEXT2));
	assert(buffer.length2() == 15);
	assert(buffer.length3() == 2);

	buffer.setCursor2(5);
	assert(buffer.cursor() == 5);
	assert(buffer.cursor2() == 5);
	assert(buffer.cursor3() == 0);

	buffer.setCursor2(12);
	assert(buffer.cursor() == 12);
	assert(buffer.cursor2() == 12);
	assert(buffer.cursor3() == 1);

	buffer.setCursor2(13);
	assert(buffer.cursor() == 15);
	assert(buffer.cursor2() == 13);
	assert(buffer.cursor3() == 1);

	buffer.setCursor(buffer.length());
	buffer.moveCursor(-1);
	assert(buffer.cursor() == buffer.length() - 1);
	assert(buffer.cursor2() == 14);
	assert(buffer.cursor3() == 1);

	buffer.setCursor3(0);
	assert(buffer.cursor() == 0);
	assert(buffer.cursor2() == 0);
	assert(buffer.cursor3() == 0);

	buffer.setCursor3(2);
	assert(buffer.cursor() == buffer.length());
	assert(buffer.cursor2() == buffer.length2());
	assert(buffer.cursor3() == 2);

	buffer.setCursor3(1);
	assert(static_cast<unsigned>(buffer.cursor()) == strlen(TEXT1));
	assert(static_cast<unsigned>(buffer.cursor2()) == strlen(TEXT1));
	assert(buffer.cursor3() == 1);

	buffer.getAtoms(buffer.cursor(), buffer.length() - buffer.cursor(), text);
	assert(memcmp(TEXT2, text, strlen(TEXT2)) == 0);
}

#endif // #ifdef SMYD_TEXT_BUFFER_UNIT_TEST
