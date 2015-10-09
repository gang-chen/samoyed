// Miscellaneous utilities.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_MISCELLANEOUS_HPP
#define SMYD_MISCELLANEOUS_HPP

#include <string.h>
#include <string>
#include <glib.h>
#include <gio/gio.h>
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
const int TEXT_WIDTH_REQUEST = 400;
const int TEXT_HEIGHT_REQUEST = 300;

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

/**
 * UNIX time.
 */
struct Time
{
    guint64 seconds;
    guint32 microSeconds;
};

int numberOfProcessors();

bool removeFileOrDirectory(const char *name, GError **error);

const char **characterEncodings();

void gtkMessageDialogAddDetails(GtkWidget *dialog, const char *details, ...);

enum SpawnSubprocessFlag
{
    SPAWN_SUBPROCESS_FLAG_NONE,
    SPAWN_SUBPROCESS_FLAG_STDIN_PIPE        = 1,
    SPAWN_SUBPROCESS_FLAG_STDOUT_PIPE       = 1 << 1,
    SPAWN_SUBPROCESS_FLAG_STDOUT_SILENCE    = 1 << 2,
    SPAWN_SUBPROCESS_FLAG_STDERR_PIPE       = 1 << 3,
    SPAWN_SUBPROCESS_FLAG_STDERR_SILENCE    = 1 << 4,
    SPAWN_SUBPROCESS_FLAG_STDERR_MERGE      = 1 << 5
};

bool spawnSubprocess(const char *cwd,
                     char **argv,
                     char **env,
                     unsigned int flags,
                     GPid *subprocessId,
                     GOutputStream **stdinPipe,
                     GInputStream **stdoutPipe,
                     GInputStream **stderrPipe,
                     GError **error);

}

#endif
