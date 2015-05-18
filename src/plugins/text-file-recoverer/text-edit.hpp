// Various text edits.
// Copyright (C) 2013 Gang Chen.

#ifndef SMYD_TXRC_TEXT_EDIT_HPP
#define SMYD_TXRC_TEXT_EDIT_HPP

#include <stdio.h>
#include <string.h>
#include <string>
#include <boost/shared_ptr.hpp>
#include <glib.h>

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
    virtual ~TextEdit() {}

    virtual bool merge(const TextInsertion *ins) { return false; }
    virtual bool merge(const TextRemoval *rem) { return false; }

    virtual bool write(FILE *file) const = 0;

    static bool replay(TextFile &file, const char *&byteCode, int &length);
};

class TextInit: public TextEdit
{
public:
    TextInit(boost::shared_ptr<char> text, int length):
        m_text(text), m_length(length)
    {
        if (m_length == -1)
            m_length = strlen(m_text.get());
    }

    virtual bool write(FILE *file) const;

    static bool replay(TextFile &file, const char *&byteCode, int &length);

private:
    boost::shared_ptr<char> m_text;
    int m_length;
};

class TextInsertion: public TextEdit
{
public:
    TextInsertion(int line, int column, const char *text, int length):
        m_line(line), m_column(column), m_text(text, length)
    {}

    virtual bool merge(const TextInsertion *ins);

    virtual bool write(FILE *file) const;

    static bool replay(TextFile &file, const char *&byteCode, int &length);

private:
    int m_line;
    int m_column;
    std::string m_text;
};

class TextRemoval: public TextEdit
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

    virtual bool merge(const TextRemoval *rem);

    virtual bool write(FILE *file) const;

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
