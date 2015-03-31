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

LockFile::State LockFile::queryState()
{
    int fd, l, m = 0, n = 256;
    char *cp;

    if (m_locked)
        return STATE_LOCKED_BY_THIS_LOCK;

    m_lockingHostName.clear();
    m_lockingProcessId = -1;

    fd = g_open(m_fileName.c_str(), O_RDONLY, 0);
    if (fd == -1)
    {
        if (errno == ENOENT)
            return STATE_UNLOCKED;
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
    }

    boost::scoped_array<char> buffer(new char[n]);
    for (;;)
    {
        l = read(fd, buffer.get() + m, n - m - 1);
        if (l == -1)
        {
            close(fd);
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        }
        if (l == 0)
            break;
        m += l;
        if (n - m - 1 == 0)
        {
            n = n * 2;
            buffer.reset(new char[n]);
        }
    }
    close(fd);
    buffer.get()[m] = '\0';
    cp = strchr(buffer.get(), ' ');
    if (cp == NULL)
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
    *cp = '\0';
    m_lockingHostName = buffer.get();
    m_lockingProcessId = atoi(cp + 1);

    if (m_lockingHostName != g_get_host_name())
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
    if (m_lockingProcessId == processId())
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
    return STATE_UNLOCKED;
}

LockFile::State LockFile::lock()
{
    int fd, l, m = 0, n = 256;
    char *buffer;
    char *cp;

    assert(!m_locked);
    m_lockingHostName.clear();
    m_lockingProcessId = -1;

    const char *thisHostName = g_get_host_name();
    ProcessId thisProcessId = processId();

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

        boost::scoped_array<char> buffer(new char[n]);
        for (;;)
        {
            l = read(fd, buffer.get() + m, n - m - 1);
            if (l == -1)
            {
                close(fd);
                return STATE_LOCKED_BY_ANOTHER_PROCESS;
            }
            if (l == 0)
                break;
            m += l;
            if (n - m - 1 == 0)
            {
                n = n * 2;
                buffer.reset(new char[n]);
            }
        }
        if (close(fd))
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        buffer.get()[m] = '\0';
        cp = strchr(buffer.get(), ' ');
        if (cp == NULL)
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        *cp = '\0';
        m_lockingHostName = buffer.get();
        m_lockingProcessId = atoi(cp + 1);

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
    buffer = g_strdup_printf("%s " PROCESS_ID_FORMAT_STR,
                             thisHostName, thisProcessId);
    l = write(fd, buffer, strlen(buffer));
    if (close(fd) || l == -1)
    {
        g_unlink(m_fileName.c_str());
        g_free(buffer);
        return STATE_FAILED;
    }
    g_free(buffer);
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
    Samoyed::LockFile::ProcessId thisProcessId = processId();
    char *buffer;

    {
        Samoyed::LockFile l(fileName);
        assert(l.queryState() == Samoyed::LockFile::STATE_UNLOCKED);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_THIS_LOCK);
        assert(strcmp(l.lockingHostName(), thisHostName) == 0);
        assert(l.lockingProcessId() == thisProcessId);
    }

    buffer = g_strdup_printf("%s " PROCESS_ID_FORMAT_STR,
                             thisHostName, thisProcessId);
    if (g_file_set_contents(fileName, buffer, -1, NULL))
    {
        g_free(buffer);
        Samoyed::LockFile l(fileName);
        assert(l.queryState() ==
               Samoyed::LockFile::STATE_LOCKED_BY_THIS_PROCESS);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_THIS_PROCESS);
        assert(strcmp(l.lockingHostName(), thisHostName) == 0);
        assert(l.lockingProcessId() == thisProcessId);
    }

#ifndef OS_WINDOWS
    buffer = g_strdup_printf("%s 0", thisHostName);
    if (g_file_set_contents(fileName, buffer, -1, NULL))
    {
        g_free(buffer);
        Samoyed::LockFile l(fileName);
        assert(l.queryState() ==
               Samoyed::LockFile::STATE_LOCKED_BY_ANOTHER_PROCESS);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_ANOTHER_PROCESS);
        assert(strcmp(l.lockingHostName(), thisHostName) == 0);
        assert(l.lockingProcessId() == 0);
    }
#endif

    buffer = g_strdup_printf("%s 1000000", thisHostName);
    if (g_file_set_contents(fileName, buffer, -1, NULL))
    {
        g_free(buffer);
        Samoyed::LockFile l(fileName);
        assert(l.queryState() == Samoyed::LockFile::STATE_UNLOCKED);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_THIS_LOCK);
        assert(strcmp(l.lockingHostName(), thisHostName) == 0);
        assert(l.lockingProcessId() == thisProcessId);
    }

    return 0;
}

#endif
