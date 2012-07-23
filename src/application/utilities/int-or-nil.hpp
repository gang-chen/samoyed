// Integer or nil.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_INT_OR_NIL_HPP
#define SMYD_INT_OR_NIL_HPP

namespace Samoyed
{

template<bool INT> class IntOrNil
{
public:
    IntOrNil(): m_i(0) {}
    IntOrNil(int i): m_i(i) {}
    int operator=(int i) { return m_i = i; }
    int operator+=(int i) { return m_i += i; }
    int operator-=(int i) { return m_i -= i; }
    int operator++() { return ++m_i; }
    int operator--() { return --m_i; }
    operator int() const { return m_i; }
private:
    int m_i;
};

template<> class IntOrNil<false>
{
public:
    IntOrNil() {}
    IntOrNil(int i) {}
    int operator=(int i) { return i; }
    int operator+=(int i) { return i; }
    int operator-=(int i) { return -i; }
    int operator++() { return 0; }
    int operator--() { return 0; }
    operator int() const { return 0; }
};

}

#endif
