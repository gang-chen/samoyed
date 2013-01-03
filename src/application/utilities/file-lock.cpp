// File lock.
// Copyright (C) 2012 Gang Chen.

#include "file-lock.hpp"
#include "signal.hpp"

namespace Samoyed
{

FileLock *FileLock::s_first = NULL, *FileLock::s_last = NULL;

FileLock::FileLock(const char *fileName):
    m_fileName(fileName),
    m_locked(false)
{
    addToList(s_first, s_last);
}

FileLock::~FileLock()
{
    removeFromList(s_first, s_last);
    if (m_locked)
        unlocked();
}

FileLock::LockedState FileLock::locked() const
{
    if (m_locked)
        return LOCKED_BY_THIS_LOCK;

}

void FileLock::lock()
{
}

}
