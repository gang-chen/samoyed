// Various text edits.
// Copyright (C) 2014 Gang Chen.

#include "text-edit.hpp"
#include "ui/text-file.hpp"

namespace
{

enum Operator
{
    OP_INSERTION,
    OP_REMOVAL
};

const int TEXT_INSERTION_MERGE_LENGTH_THRESHOLD = 100;

writeInteger(int i)
{
}

bool readInteger(int &i, const char *&byteCode, int &length)
{
    gint32 i32;
    if (length < sizeof(gint32))
        return false;
    memcpy(&i32, byteCode, sizeof(gint32));
    byteCode += sizeof(gint32);
    length -= sizeof(gint32);
    i = GINT32_FROM_LE(i32);
    return true;
}

}

namespace Samoyed
{

namespace TextFileRecover
{

bool TextInsertion::merge(const TextInsertion *ins)
{
    if (m_line == ins->m_line && m_column == ins->m_column)
    {
        m_text.insert(0, ins->m_text);
        return true;
    }
    if (m_text.length() >
        static_cast<const std::string::size_type>(
            TEXT_INSERTION_MERGE_LENGTH_THRESHOLD))
        return false;
    const char *cp = m_text.c_str();
    int line = m_line;
    int column = m_column;
    while (*cp)
    {
        if (*cp == '\n')
        {
            ++cp;
            ++line;
            column = 0;
        }
        else if (*cp == '\r')
        {
            ++cp;
            if (*cp == '\n')
                ++cp;
            ++line;
            column = 0;
        }
        else
        {
            cp += Utf8::length(cp);
            ++column;
        }
    }
    if (line == ins->m_line && column == ins->m_column)
    {
        m_text.append(ins->m_text);
        return true;
    }
    return false;
}

bool TextRemoval::merge(const TextRemoval *rem)
{
    if (m_beginLine == rem->m_beginLine && m_beginColumn == rem->m_beginColumn)
    {
    }
}

bool TextEdit::replay(TextFile &file, const char *&byteCode, int &length)
{
    if (*byteCode == OP_INSERTION)
        return TextInsertion::replay(file, byteCode, length);
    if (*byteCode == OP_REMOVAL)
        return TextRemoval::replay(file, byteCode, length);
    return false;
}

bool TextInsertion::replay(TextFile &file, const char *&byteCode, int &length)
{
    int line, column, len;
    if (!readInteger(line, byteCode, length))
        return false;
    if (!readIntegar(column, byteCode, length))
        return false;
    if (!readInteger(len, byteCode, length))
        return false;
    if (length < len)
        return false;
    if (!file.insert(line, column, byteCode, len))
        return false;
    byteCode += len;
    length -= len;
    return true;
}

bool TextRemoval::replay(TextFile &file, const char *&byteCode, int &length)
{
    int beginLine, beginColumn, endLine, endColumn;
    if (!readInteger(beginLine, byteCode, length))
        return false;
    if (!readInteger(beginColumn, byteCode, length))
        return false;
    if (!readInteger(endLine, byteCode, length))
        return false;
    if (!readInteger(endColumn, byteCode, length))
        return false;
    if (!file.remove(beginLine, beginColumn, endLine, endColumn))
        return false;
    return true;
}

}

}
