// Symbol declaration.
// Copyright (C) 2015 Gang Chen.

#ifndef SMYD_SYMBOL_DECLARATION_HPP
#define SMYD_SYMBOL_DECLARATION_HPP

namespace Samoyed
{

class SymbolDeclaration
{
public:
    enum Kind
    {
    };

    enum Attribute
    {
    };

    virtual int size() const;
    virtual int serialize(void *data) const;

private:
    char *m_symbol;

    int m_line;
    int m_column;
};

}

#endif
