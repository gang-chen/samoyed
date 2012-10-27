// Miscellaneous utilities.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_MISC_HPP
#define SMYD_MISC_HPP

#include <string.h>
#include <string>

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

template<class Pointer> class ComparablePointer
{
public:
    ComparablePointer(Pointer pointer): m_pointer(pointer) {}
    operator Pointer() const { return m_pointer; }
    bool operator<(const ComparablePointer<Pointer> cp) const
    { return *m_pointer < *cp.m_pointer; }
private:
    Pointer m_pointer;
};

template<> class ComparablePointer<const char *>
{
public:
    ComparablePointer(const char *pointer): m_pointer(pointer) {}
    operator const char *() const { return m_pointer;}
    bool operator<(const ComparablePointer<const char *> cp) const
    { return strcmp(m_pointer, cp.m_pointer) < 0; }
private:
    const char *m_pointer;
};

class CastableString
{
public:
    CastableString() {}
    CastableString(const char *s): m_string(s) {}
    operator const char *() const { return m_string.c_str(); }
private:
    std::string m_string;
};

}

#endif
