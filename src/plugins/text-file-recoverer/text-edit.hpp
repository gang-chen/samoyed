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
        TYPE_REMOVAL,
        TYPE_INSERTION_AT_CURSOR,
        TYPE_INSERTION_ONE,
        TYPE_INSERTION_ONE_AT_CURSOR,
        TYPE_REMOVAL_AT_CURSOR,
        TYPE_REMOVAL_ONE_BEFORE_CURSOR,
        TYPE_REMOVAL_ONE_AFTER_CURSOR
    };
    char type;
    TextEdit *next;
    TextEdit(t): type(t), next(NULL) {}
};

struct TextInsertion: public TextEdit
{
    int line;
    int column;
    const char *text;
    int length;
    TextInsertion(): TextEdit(TYPE_INSERTION) {}
};

struct TextRemoval: public TextEdit
{
    int beginLine;
    int beginColumn;
    int endLine;
    int endColumn;
    TextRemoval(): TextEdit(TYPE_REMOVAL) {}
};

struct TextInsertionAtCursor: public TextEdit
{
    const char *text;
    int length;
    TextInsertionAtCursor(): TextEdit(TYPE_INSERTION_AT_CURSOR) {}
};

struct TextInsertionOne: public TextEdit
{
    int beginLine;
    int beginColumn;
    const char *text;
    TextInsertionOne(): TextEdit(TYPE_INSERTION_ONE) {}
};

struct TextInsertionOneAtCursor: public TextEdit
{
    const char *text;
    TextInsertionOneAtCursor(): TextEdit(TYPE_INSERTION_ONE_AT_CURSOR) {}
};

struct TextRemovalAtCursor: public TextEdit
{
    int endLine;
    int endColumn;
    TextRemovalAtCursor(): TextEdit(TYPE_REMOVAL_AT_CURSOR) {}
};

struct TextRemovalOneBeforeCursor: public TextEdit
{
    TextRemovalOneBeforeCursor(): TextEdit(TYPE_REMOVAL_ONE_BEFORE_CURSOR) {}
};

struct TextRemovalOneAfterCursor: public TextEdit
{
    TextRemovalOneAfterCursor(): TextEdit(TYPE_REMOVAL_ONE_AFTER_CURSOR) {}
};

}

}

#endif
