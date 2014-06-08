// Various text edits.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_EDIT_HPP
#define SMYD_TXTR_TEXT_EDIT_HPP

#include <string>

namespace Samoyed
{

class TextFile;

namespace TextFileRecoverer
{

class TextInsertion;
class TextRemoval;

class TextEdit
{
public:
    virtual bool merge(const TextInsertion *ins) { return false; }
    virtual bool merge(const TextRemoval *rem) { return false; }

    virtual void write(char *&byteCode, int &length) const = 0;

    static bool replay(TextFile &file, const char *&byteCode, int &length);
};

class TextInsertion: public TextEdit
{
public:
    TextInsertion(int line, int column, const char *text):
        m_line(line), m_column(column), m_text(text)
    {}

    virtual void write(char *&byteCode, int &length) const = 0;

    static bool replay(TextFile &file, const char *&byteCode, int &length);

private:
    int m_line;
    int m_column;
    std::string m_text;
};

struct TextRemoval: public TextEdit
{
public:
    TextRemoval(int beginLine,
                int beginColumn,
                int endLine,
                int endColumn):
        m_beginLine(beginLine),
        m_beginColumn(beginColumn),
        m_endLine(endLine),
        m_endColumn(endColumn)
    {}

    virtual void write(char *&byteCode, int &length) const = 0;

    static bool replay(TextFile &file, const char *&byteCode, int &length);

private:
    int m_beginLine;
    int m_beginColumn;
    int m_endLine;
    int m_endColumn;
};

}

}

#endif
