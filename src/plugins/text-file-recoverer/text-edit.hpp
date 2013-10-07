// Various text edits.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_EDIT_HPP
#define SMYD_TXTR_TEXT_EDIT_HPP

namespace Samoyed
{

namespace TextFileRecoverer
{

struct TextEdit
{
    enum Type
    {
        TYPE_INSERTION,
        TYPE_INSERTION_1,
        TYPE_INSERTION_2,
        TYPE_INSERTION_3,
        TYPE_INSERTION_4,
        TYPE_INSERTION_5,
        TYPE_INSERTION_6,
        TYPE_INSERTION_7,
        TYPE_INSERTION_8,
        TYPE_INSERTION_9,
        TYPE_INSERTION_10,
        TYPE_INSERTION_AT_CURSOR,
        TYPE_INSERTION_1_AT_CURSOR,
        TYPE_INSERTION_2_AT_CURSOR,
        TYPE_INSERTION_3_AT_CURSOR,
        TYPE_INSERTION_4_AT_CURSOR,
        TYPE_INSERTION_5_AT_CURSOR,
        TYPE_INSERTION_6_AT_CURSOR,
        TYPE_INSERTION_7_AT_CURSOR,
        TYPE_INSERTION_8_AT_CURSOR,
        TYPE_INSERTION_9_AT_CURSOR,
        TYPE_INSERTION_10_AT_CURSOR,
        TYPE_REMOVAL,
        TYPE_REMOVAL_BEFORE_CURSOR,
        TYPE_REMOVAL_1_BEFORE_CURSOR,
        TYPE_REMOVAL_2_BEFORE_CURSOR,
        TYPE_REMOVAL_3_BEFORE_CURSOR,
        TYPE_REMOVAL_4_BEFORE_CURSOR,
        TYPE_REMOVAL_5_BEFORE_CURSOR,
        TYPE_REMOVAL_6_BEFORE_CURSOR,
        TYPE_REMOVAL_7_BEFORE_CURSOR,
        TYPE_REMOVAL_8_BEFORE_CURSOR,
        TYPE_REMOVAL_9_BEFORE_CURSOR,
        TYPE_REMOVAL_10_BEFORE_CURSOR,
        TYPE_REMOVAL_AFTER_CURSOR,
        TYPE_REMOVAL_1_AFTER_CURSOR,
        TYPE_REMOVAL_2_AFTER_CURSOR,
        TYPE_REMOVAL_3_AFTER_CURSOR,
        TYPE_REMOVAL_4_AFTER_CURSOR,
        TYPE_REMOVAL_5_AFTER_CURSOR,
        TYPE_REMOVAL_6_AFTER_CURSOR,
        TYPE_REMOVAL_7_AFTER_CURSOR,
        TYPE_REMOVAL_8_AFTER_CURSOR,
        TYPE_REMOVAL_9_AFTER_CURSOR,
        TYPE_REMOVAL_10_AFTER_CURSOR
    };
    char type;
    TextEdit(char t): type(t) {}
};

struct TextInsertion: public TextEdit
{
    int line;
    int column;
    int length;
    char text[1];
    TextInsertion(): TextEdit(TYPE_INSERTION) {}
};

struct TextInsertionX: public TextEdit
{
    int line;
    int column;
    char text[1];
    TextInsertionX(char t): TextEdit(t) {}
};

struct TextInsertionAtCursor: public TextEdit
{
    int length;
    char text[1];
    TextInsertionAtCursor(): TextEdit(TYPE_INSERTION_AT_CURSOR) {}
};

struct TextInsertionXAtCursor: public TextEdit
{
    char text[1];
    TextInsertionXAtCursor(char t): TextEdit(t) {}
};

struct TextRemoval: public TextEdit
{
    int line;
    int column;
    int length;
    TextRemoval(): TextEdit(TYPE_REMOVAL) {}
};

struct TextRemovalBeforeCursor: public TextEdit
{
    int length;
    TextRemovalBeforeCursor(): TextEdit(TYPE_REMOVAL_BEFORE_CURSOR) {}
};

struct TextRemovalXBeforeCursor: public TextEdit
{
    TextRemovalXBeforeCursor(char t): TextEdit(t) {}
};

struct TextRemovalAfterCursor: public TextEdit
{
    int length;
    TextRemovalAfterCursor(): TextEdit(TYPE_REMOVAL_AFTER_CURSOR) {}
};

struct TextRemovalXAfterCursor: public TextEdit
{
    TextRemovalOneAfterCursor(char t): TextEdit(t) {}
};

}

}

#endif
