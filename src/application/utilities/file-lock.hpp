// File lock.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_FILE_LOCK_HPP
#define SMYD_FILE_LOCK_HPP

#include "misc.hpp"
#include <string>

namespace Samoyed
{

class FileLock
{
public:
    enum LockedState
    {
        UNLOCKED,
        LOCKED_BY_THIS_LOCK,
        LOCKED_BY_THIS_PROCESS,
        LOCKED_BY_ANOTHER_PROCESS
    };

    FileLock(const char *fileName);

    ~FileLock();

    LockedState locked() const;

    void lock();

    bool tryLock();

    void unlock();

private:
    static void onCrashed();

    static FileLock *s_first, *s_last;

    std::string m_fileName;

    bool m_locked;

    SMYD_DEFINE_DOUBLY_LINKED(FileLock)
};

}

#endif
