// Lock file.
// Copyright (C) 2012 Gang Chen.

#ifndef SMYD_LOCK_FILE_HPP
#define SMYD_LOCK_FILE_HPP

#include "miscellaneous.hpp"
#include <string>
#ifdef OS_WINDOWS
# include <windows.h>
#else
# include <unistd.h>
#endif

namespace Samoyed
{

class LockFile
{
public:
#ifdef OS_WINDOWS
    typedef DWORD ProcessId;
#else
    typedef pid_t ProcessId;
#endif

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

    ProcessId lockingProcessId() const { return m_lockingProcessId; }

private:
    static void onCrashed(int signalNumber);

    const std::string m_fileName;

    bool m_locked;

    std::string m_lockingHostName;

    ProcessId m_lockingProcessId;

    SAMOYED_DEFINE_DOUBLY_LINKED(LockFile)
};

}

#endif
