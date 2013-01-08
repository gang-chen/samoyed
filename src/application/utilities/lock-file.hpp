// Lock file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_LOCK_FILE_HPP
#define SMYD_LOCK_FILE_HPP

#include "misc.hpp"
#include <string>
#include <unistd.h>

namespace Samoyed
{

class LockFile
{
public:
    enum State
    {
        STATE_FAILED,
        STATE_UNLOCKED,
        STATE_LOCKED_BY_THIS_LOCK,
        STATE_LOCKED_BY_THIS_PROCESS,
        STATE_LOCKED_BY_ANOTHER_PROCESS
    };

    LockFile(const char *fileName);

    ~LockFile();

    State queryState();

    State lock();

    void unlock(bool force);

    const char *fileName() const { return m_fileName.c_str(); }

    const char *lockingHostName() const { return m_lockingHostName.c_str(); }

    pid_t lockingProcessId() const { return m_lockingProcessId; }

private:
    static void onCrashed(int signalNumber);

    static bool s_crashHandlerRegistered;

    static LockFile *s_first, *s_last;

    const std::string m_fileName;

    bool m_locked;

    std::string m_lockingHostName;

    pid_t m_lockingProcessId;

    SMYD_DEFINE_DOUBLY_LINKED(LockFile)
};

}

#endif
