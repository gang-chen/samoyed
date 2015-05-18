// Lock file.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ lock-file.cpp signal.cpp -DSMYD_LOCK_FILE_UNIT_TEST \
`pkg-config --cflags --libs gtk+-3.0` -Werror -Wall -o lock-file
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "lock-file.hpp"
#include "miscellaneous.hpp"
#include "signal.hpp"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#ifdef SMYD_LOCK_FILE_UNIT_TEST
# include <string.h>
#endif
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <boost/scoped_array.hpp>
#ifdef SMYD_LOCK_FILE_UNIT_TEST
# include <boost/scoped_ptr.hpp>
#endif
#include <glib.h>
#include <glib/gstdio.h>

namespace
{

bool crashHandlerRegistered = false;

Samoyed::LockFile *first = NULL, *last = NULL;

#ifdef OS_WINDOWS
# define PROCESS_ID_FORMAT_STR "%ld"
#else
# define PROCESS_ID_FORMAT_STR "%d"
#endif

Samoyed::LockFile::ProcessId processId()
{
#ifdef OS_WINDOWS
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

bool readLockFile(int fd,
                  std::string &lockingHostName,
                  Samoyed::LockFile::ProcessId &lockingProcessId)
{
    int l, m = 0, n = 256;
    char *t;
    boost::scoped_array<char> buffer;
    buffer.reset(new char[n]);
    for (;;)
    {
        l = read(fd, buffer.get() + m, n - m - 1);
        if (l == -1)
            return false;
        if (l == 0)
            break;
        m += l;
        if (n - m - 1 == 0)
        {
            n = n * 2;
            t = new char[n];
            memcpy(t, buffer.get(), sizeof(char) * m);
            buffer.reset(t);
        }
    }
    buffer.get()[m] = '\0';
    t = strchr(buffer.get(), ' ');
    if (t == NULL)
        return false;
    *t = '\0';
    lockingHostName = buffer.get();
    lockingProcessId = atoi(t + 1);
    return true;
}

}

namespace Samoyed
{

void LockFile::onCrashed(int signalNumber)
{
    for (LockFile *l = first; l; l = l->next())
    {
        if (l->m_locked)
            l->unlock(false);
    }
}

LockFile::LockFile(const char *fileName):
    m_fileName(fileName),
    m_locked(false),
    m_lockingProcessId(-1)
{
    if (!crashHandlerRegistered)
    {
        Signal::registerCrashHandler(onCrashed);
        crashHandlerRegistered = true;
    }
    addToList(first, last);
}

LockFile::~LockFile()
{
    if (m_locked)
        unlock(false);
    removeFromList(first, last);
}

LockFile::State LockFile::queryState(const char *fileName,
                                     std::string *lockingHostName,
                                     ProcessId *lockingProcessId)
{
    int fd;
    std::string host;
    ProcessId pid;

    if (lockingHostName)
        lockingHostName->clear();
    if (lockingProcessId)
        *lockingProcessId = -1;

    fd = g_open(fileName, O_RDONLY, 0);
    if (fd == -1)
    {
        if (errno == ENOENT)
            return STATE_UNLOCKED;
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
    }
    if (!readLockFile(fd, host, pid))
    {
        close(fd);
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
    }
    close(fd);

    if (lockingHostName)
        *lockingHostName = host;
    if (lockingProcessId)
        *lockingProcessId = pid;

    if (host != g_get_host_name())
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
    if (pid == processId())
        return STATE_LOCKED_BY_THIS_PROCESS;

    // Check to see if the lock file is stale.
#ifdef OS_WINDOWS
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION,
                           FALSE,
                           pid);
    if (h)
    {
        CloseHandle(h);
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
    }
    if (GetLastError() == ERROR_ACCESS_DENIED)
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
#else
    if (!kill(pid, 0) || errno != ESRCH)
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
#endif

    // Remove the stale lock file.
    if (g_unlink(fileName))
        return STATE_LOCKED_BY_ANOTHER_PROCESS;

    if (lockingHostName)
        lockingHostName->clear();
    if (lockingProcessId)
        *lockingProcessId = -1;
    return STATE_UNLOCKED;
}

LockFile::State LockFile::lock()
{
    int fd;

    const char *thisHostName = g_get_host_name();
    ProcessId thisProcessId = processId();

    assert(!m_locked);
    m_lockingHostName.clear();
    m_lockingProcessId = -1;

RETRY:
    fd = g_open(m_fileName.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd == -1)
    {
        // The lock file doesn't exist but we can't create it.
        if (errno != EEXIST)
            return STATE_FAILED;

        // Read the locking host name and process ID.  Don't treat a read
        // failure as a hard failure because another process may be writing the
        // lock file.
        fd = g_open(m_fileName.c_str(), O_RDONLY, 0);
        if (fd == -1)
        {
            if (errno == ENOENT)
                // The lock file doesn't exist.  Retry to create it.
                goto RETRY;
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        }
        if (!readLockFile(fd, m_lockingHostName, m_lockingProcessId))
        {
            close(fd);
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        }
        close(fd);

        if (m_lockingHostName != thisHostName)
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        if (m_lockingProcessId == thisProcessId)
            return STATE_LOCKED_BY_THIS_PROCESS;

        // Check to see if the lock file is stale.
#ifdef OS_WINDOWS
        HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION,
                               FALSE,
                               m_lockingProcessId);
        if (h)
        {
            CloseHandle(h);
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        }
        if (GetLastError() == ERROR_ACCESS_DENIED)
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
#else
        if (!kill(m_lockingProcessId, 0) || errno != ESRCH)
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
#endif

        // Remove the stale lock file.
        if (g_unlink(m_fileName.c_str()))
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        m_lockingHostName.clear();
        m_lockingProcessId = -1;
        goto RETRY;
    }

    // Write our host name and process ID.
    char *buffer = g_strdup_printf("%s " PROCESS_ID_FORMAT_STR,
                                   thisHostName, thisProcessId);
    int l = write(fd, buffer, strlen(buffer));
    g_free(buffer);
    if (close(fd) || l == -1)
    {
        g_unlink(m_fileName.c_str());
        return STATE_FAILED;
    }
    m_locked = true;
    m_lockingHostName = thisHostName;
    m_lockingProcessId = thisProcessId;
    return STATE_LOCKED_BY_THIS_LOCK;
}

void LockFile::unlock(bool force)
{
    if (!force)
        assert(m_locked);
    if (!g_unlink(m_fileName.c_str()))
        m_locked = false;
}

}

#ifdef SMYD_LOCK_FILE_UNIT_TEST

int main()
{
    const char *fileName = "lock-file-unit-test";
    const char *thisHostName = g_get_host_name();
    boost::scoped_ptr<char> buffer;
    Samoyed::LockFile::ProcessId thisProcessId = processId();

    {
        Samoyed::LockFile l(fileName);
        assert(Samoyed::LockFile::queryState(fileName, NULL, NULL) ==
               Samoyed::LockFile::STATE_UNLOCKED);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_THIS_LOCK);
        assert(strcmp(l.lockingHostName(), thisHostName) == 0);
        assert(l.lockingProcessId() == thisProcessId);
    }

    buffer.reset(g_strdup_printf("%s " PROCESS_ID_FORMAT_STR,
                                 thisHostName, thisProcessId),
                 g_free);
    if (g_file_set_contents(fileName, buffer.get(), -1, NULL))
    {
        Samoyed::LockFile l(fileName);
        assert(Samoyed::LockFile::queryState(fileName, NULL, NULL) ==
               Samoyed::LockFile::STATE_LOCKED_BY_THIS_PROCESS);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_THIS_PROCESS);
        assert(strcmp(l.lockingHostName(), thisHostName) == 0);
        assert(l.lockingProcessId() == thisProcessId);
    }

#ifndef OS_WINDOWS
    buffer.reset(g_strdup_printf("%s 0", thisHostName), g_free);
    if (g_file_set_contents(fileName, buffer.get(), -1, NULL))
    {
        Samoyed::LockFile l(fileName);
        assert(Samoyed::LockFile::queryState(fileName, NULL, NULL) ==
               Samoyed::LockFile::STATE_LOCKED_BY_ANOTHER_PROCESS);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_ANOTHER_PROCESS);
        assert(strcmp(l.lockingHostName(), thisHostName) == 0);
        assert(l.lockingProcessId() == 0);
    }
#endif

    buffer.reset(g_strdup_printf("%s 1000000", thisHostName), g_free);
    if (g_file_set_contents(fileName, buffer.get(), -1, NULL))
    {
        Samoyed::LockFile l(fileName);
        assert(Samoyed::LockFile::queryState(fileName, NULL, NULL) ==
               Samoyed::LockFile::STATE_UNLOCKED);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_THIS_LOCK);
        assert(strcmp(l.lockingHostName(), thisHostName) == 0);
        assert(l.lockingProcessId() == thisProcessId);
    }

    return 0;
}

#endif
