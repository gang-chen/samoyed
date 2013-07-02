// Miscellaneous utilities.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_MISCELLANEOUS_HPP
#define SMYD_MISCELLANEOUS_HPP

#include <string.h>
#include <string>
#include <gtk/gtk.h>

#define SAMOYED_DEFINE_DOUBLY_LINKED(Class) \
public: \
    Class *next() { return m_next; } \
    Class *previous() { return m_previous; } \
    const Class *next() const { return m_next; } \
    const Class *previous() const { return m_previous; } \
    void addToList(Class *&first, Class *&last) \
    { \
        m_previous = last; \
        m_next = NULL; \
        if (last) \
            last->m_next = this; \
        else \
            first = this; \
        last = this; \
    } \
    void removeFromList(Class *&first, Class *&last) \
    { \
        if (m_previous) \
            m_previous->m_next = m_next; \
        else \
            first = m_next; \
        if (m_next) \
            m_next->m_previous = m_previous; \
        else \
            last = m_previous; \
    } \
private: \
    Class *m_next; \
    Class *m_previous;

#define SAMOYED_DEFINE_DOUBLY_LINKED_IN(Class, Container) \
public: \
    Class *nextIn##Container() { return m_nextIn##Container; } \
    Class *previousIn##Container() { return m_previousIn##Container; } \
    const Class *nextIn##Container() const { return m_nextIn##Container; } \
    const Class *previousIn##Container() const \
    { return m_previousIn##Container; } \
    void addToListIn##Container(Class *&first, Class *&last) \
    { \
        m_previousIn##Container = last; \
        m_nextIn##Container = NULL; \
        if (last) \
            last->m_nextIn##Container = this; \
        else \
            first = this; \
        last = this; \
    } \
    void removeFromListIn##Container(Class *&first, Class *&last) \
    { \
        if (m_previousIn##Container) \
            m_previousIn##Container->m_nextIn##Container = \
                m_nextIn##Container; \
        else \
            first = m_nextIn##Container; \
        if (m_nextIn##Container) \
            m_nextIn##Container->m_previousIn##Container = \
                m_previousIn##Container; \
        else \
            last = m_previousIn##Container; \
    } \
private: \
    Class *m_nextIn##Container; \
    Class *m_previousIn##Container;

namespace Samoyed
{

const int CONTAINER_BORDER_WIDTH = 12;
const int CONTAINER_SPACING = 6;
const int LABEL_SPACING = 12;
const int TEXT_WIDTH_REQUEST = 400;
const int TEXT_HEIGHT_REQUEST = 300;

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

template<class T> class ComparablePointer
{
public:
    ComparablePointer(T *pointer): m_pointer(pointer) {}
    operator T *() const { return m_pointer; }
    T &operator*() const { return *m_pointer; }
    T *operator->() const { return m_pointer; }
    bool operator<(const ComparablePointer cp) const
    { return *m_pointer < *cp.m_pointer; }
private:
    T *m_pointer;
};

template<> class ComparablePointer<const char>
{
public:
    ComparablePointer(const char *pointer): m_pointer(pointer) {}
    operator const char *() const { return m_pointer; }
    const char &operator*() const { return *m_pointer; }
    const char *operator->() const { return m_pointer; }
    bool operator<(const ComparablePointer cp) const
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

class Orientable
{
public:
    enum Orientation
    {
        ORIENTATION_HORIZONTAL = GTK_ORIENTATION_HORIZONTAL,
        ORIENTATION_VERTICAL = GTK_ORIENTATION_VERTICAL
    };

    virtual ~Orientable() {}
    virtual Orientation orientation() const = 0;
};

int getNumberOfProcessors();

bool isValidFileName(const char *fileName);

bool removeFileOrDirectory(const char *name, GError **error);

void gtkMessageDialogAddDetails(GtkWidget *dialog, const char *details, ...);

}

#endif
