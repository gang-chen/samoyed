// Various text edits.
// Copyright (C) 2014 Gang Chen.

#include "text-edit.hpp"

namespace
{

enum Operator
{
    OP_INSERTION,
    OP_INSERTION_1,
    OP_INSERTION_2,
    OP_INSERTION_3,
    OP_INSERTION_4,
    OP_INSERTION_5,
    OP_INSERTION_6,
    OP_INSERTION_7,
    OP_INSERTION_8,
    OP_INSERTION_9,
    OP_INSERTION_10,
    OP_INSERTION_AT_CURSOR,
    OP_INSERTION_1_AT_CURSOR,
    OP_INSERTION_2_AT_CURSOR,
    OP_INSERTION_3_AT_CURSOR,
    OP_INSERTION_4_AT_CURSOR,
    OP_INSERTION_5_AT_CURSOR,
    OP_INSERTION_6_AT_CURSOR,
    OP_INSERTION_7_AT_CURSOR,
    OP_INSERTION_8_AT_CURSOR,
    OP_INSERTION_9_AT_CURSOR,
    OP_INSERTION_10_AT_CURSOR,
    OP_REMOVAL,
    OP_REMOVAL_1,
    OP_REMOVAL_2,
    OP_REMOVAL_3,
    OP_REMOVAL_4,
    OP_REMOVAL_5,
    OP_REMOVAL_6,
    OP_REMOVAL_7,
    OP_REMOVAL_8,
    OP_REMOVAL_9,
    OP_REMOVAL_10,
    OP_REMOVAL_BEFORE_CURSOR,
    OP_REMOVAL_1_BEFORE_CURSOR,
    OP_REMOVAL_2_BEFORE_CURSOR,
    OP_REMOVAL_3_BEFORE_CURSOR,
    OP_REMOVAL_4_BEFORE_CURSOR,
    OP_REMOVAL_5_BEFORE_CURSOR,
    OP_REMOVAL_6_BEFORE_CURSOR,
    OP_REMOVAL_7_BEFORE_CURSOR,
    OP_REMOVAL_8_BEFORE_CURSOR,
    OP_REMOVAL_9_BEFORE_CURSOR,
    OP_REMOVAL_10_BEFORE_CURSOR,
    OP_REMOVAL_AFTER_CURSOR,
    OP_REMOVAL_1_AFTER_CURSOR,
    OP_REMOVAL_2_AFTER_CURSOR,
    OP_REMOVAL_3_AFTER_CURSOR,
    OP_REMOVAL_4_AFTER_CURSOR,
    OP_REMOVAL_5_AFTER_CURSOR,
    OP_REMOVAL_6_AFTER_CURSOR,
    OP_REMOVAL_7_AFTER_CURSOR,
    OP_REMOVAL_8_AFTER_CURSOR,
    OP_REMOVAL_9_AFTER_CURSOR,
    OP_REMOVAL_10_AFTER_CURSOR
};

}

namespace Samoyed
{

namespace TextFileRecover
{

bool TextEdit::replay(TextFile &file, const char *&byteCode, int &length)
{
    if (*byteCode >= OP_INSERTION && *byteCode <= OP_INSERTION_10)
        return TextInsertion::replay(file, byteCode, length);
    if (*byteCode >= OP_INSERTION_AT_CURSOR &&
        *byteCode <= OP_INSERTION_10_AT_CURSOR)
        return TextInsertionAtCursor::replay(file, byteCode, length);
    if (*byteCode >= OP_REMOVAL && *byteCode <= OP_REMOVAL_10)
        return TextRemoval::replay(file, byteCode, length);
    if (*byteCode >= OP_REMOVAL_BEFORE_CURSOR &&
        *byteCode <= OP_REMOVAL_10_BEFORE_CURSOR)
        return TextRemovalBeforeCursor::replay(file, byteCode, length);
    if (*byteCode >= OP_REMOVAL_AFTER_CURSOR &&
        *byteCode <= OP_REMOVAL_10_AFTER_CURSOR)
        return TextRemovalAfterCursor::replay(file, byteCode, length);
    return false;
}

bool TextInsertion::replay(TextFile &file, const char *&byteCode, int &length)
{
    gint32 len, ln, col;
    if (*byteCode == OP_INSERTION)
    {
        if (length < sizeof(gint32))
            return false;
        memcpy(&len, byteCode, sizeof(gint32));
        byteCode += sizeof(gint32);
        length -= sizeof(gint32);
        len = GINT32_FROM_LE(len);
    }
    else
        len = *byteCode - OP_INSERTION;
    if (length < sizeof(gint32))
        return false;
    memcpy(&ln, byteCode, sizeof(gint32));
    byteCode += sizeof(gint32);
    length -= sizeof(gint32);
    ln = GINT32_FROM_LE(ln);
    if (length < sizeof(gint32))
        return false;
    memcpy(&col, byteCode, sizeof(gint32));
    byteCode += sizeof(gint32);
    length -= sizeof(gint32);
    col = GINT32_FROM_LE(col);
    if (length < len)
        return false;
    if (!file.insert(ln, col, byteCode, len))
        return false;
    byteCode += len;
    length -= len;
    return true;
}

}

}
