// Various text edits.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXTR_TEXT_EDIT_HPP
#define SMYD_TXTR_TEXT_EDIT_HPP

#include <string>

namespace Samoyed
{

namespace TextFileRecoverer
{

class TextEdit
{
public:
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

struct TextInsertionAtCursor: public TextEdit
{
public:
    TextInsertionAtCursor(const char *text): m_text(text) {}

    virtual void write(char *&byteCode, int &length) const = 0;

    static bool replay(TextFile &file, const char *&byteCode, int &length);

private:
    std::string text;
};

struct TextRemoval: public TextEdit
{
public:
    TextRemoval(int line, int column, int length):
        m_line(line), m_column(column), m_length(length)
    {}

    virtual void write(char *&byteCode, int &length) const = 0;

    static bool replay(TextFile &file, const char *&byteCode, int &length);

private:
    int m_line;
    int m_column;
    int m_length;
};

struct TextRemovalBeforeCursor: public TextEdit
{
public:
    TextRemovalBeforeCursor(int length): m_length(length) {}

    virtual void write(char *&byteCode, int &length) const = 0;

    static bool replay(TextFile &file, const char *&byteCode, int &length);

private:
    int m_length;
};

struct TextRemovalAfterCursor: public TextEdit
{
public:
    TextRemovalAfterCursor(int length): m_length(length) {}

    virtual void write(char *&byteCode, int &length) const = 0;

    static bool replay(TextFile &file, const char *&byteCode, int &length);

private:
    int m_length;
};

}

}

#endif
