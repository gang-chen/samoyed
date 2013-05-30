// Lock file.
// Copyright (C) 2012 Gang Chen.

/*
UNIT TEST BUILD
g++ lock-file.cpp signal.cpp -DSMYD_LOCK_FILE_UNIT_TEST\
 `pkg-config --cflags --libs glib-2.0` -Werror -Wall -o lock-file
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "lock-file.hpp"
#include "signal.hpp"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef SMYD_LOCK_FILE_UNIT_TEST
# include <string.h>
#endif
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>

namespace
{

const int MAX_HOST_NAME_LENGTH = 500, MAX_PROCESS_ID_LENGTH = 500;

bool crashHandlerRegistered = false;

Samoyed::LockFile *first = NULL, *last = NULL;

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
    int fd, l;
    char buffer[MAX_HOST_NAME_LENGTH + MAX_PROCESS_ID_LENGTH];
    char *cp;
    char hostName[MAX_HOST_NAME_LENGTH];

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

    l = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (l == -1)
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
    buffer[l] = '\0';
    cp = strchr(buffer, ' ');
    if (cp == NULL)
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
    *cp = '\0';
    m_lockingHostName = buffer;
    m_lockingProcessId = atoi(cp + 1);

    hostName[0] = '\0';
    gethostname(hostName, sizeof(hostName) - 1);
    hostName[sizeof(hostName) - 1] = '\0';

    if (m_lockingHostName != hostName)
        return STATE_LOCKED_BY_ANOTHER_PROCESS;
    if (m_lockingProcessId == getpid())
        return STATE_LOCKED_BY_THIS_PROCESS;

    // Check to see if the lock file is stale.
    if (!kill(m_lockingProcessId, 0) || errno != ESRCH)
        return STATE_LOCKED_BY_ANOTHER_PROCESS;

    // Remove the stale lock file.
    if (g_unlink(m_fileName.c_str()))
        return STATE_LOCKED_BY_ANOTHER_PROCESS;

    m_lockingHostName.clear();
    m_lockingProcessId = -1;
    return STATE_UNLOCKED;
}

LockFile::State LockFile::lock()
{
    char hostName[MAX_HOST_NAME_LENGTH];
    int fd, l;
    char buffer[MAX_HOST_NAME_LENGTH + MAX_PROCESS_ID_LENGTH];
    char *cp;

    assert(!m_locked);
    m_lockingHostName.clear();
    m_lockingProcessId = -1;

    hostName[0] = '\0';
    gethostname(hostName, sizeof(hostName) - 1);
    hostName[sizeof(hostName) - 1] = '\0';

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

        l = read(fd, buffer, sizeof(buffer) - 1);
        if (close(fd) || l == -1)
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        buffer[l] = '\0';
        cp = strchr(buffer, ' ');
        if (cp == NULL)
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        *cp = '\0';
        m_lockingHostName = buffer;
        m_lockingProcessId = atoi(cp + 1);

        if (m_lockingHostName != hostName)
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        if (m_lockingProcessId == getpid())
            return STATE_LOCKED_BY_THIS_PROCESS;

        // Check to see if the lock file is stale.
        if (!kill(m_lockingProcessId, 0) || errno != ESRCH)
            return STATE_LOCKED_BY_ANOTHER_PROCESS;

        // Remove the stale lock file.
        if (g_unlink(m_fileName.c_str()))
            return STATE_LOCKED_BY_ANOTHER_PROCESS;
        m_lockingHostName.clear();
        m_lockingProcessId = -1;
        goto RETRY;
    }

    // Write our host name and process ID.
    l = snprintf(buffer, sizeof(buffer), "%s %d", hostName, getpid());
    l = write(fd, buffer, l);
    if (close(fd) || l == -1)
    {
        g_unlink(m_fileName.c_str());
        return STATE_FAILED;
    }
    m_locked = true;
    m_lockingHostName = hostName;
    m_lockingProcessId = getpid();
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
    char hostName[MAX_HOST_NAME_LENGTH];
    char buffer[MAX_HOST_NAME_LENGTH + MAX_PROCESS_ID_LENGTH];

    hostName[0] = '\0';
    gethostname(hostName, sizeof(hostName) - 1);
    hostName[sizeof(hostName) - 1] = '\0';

    {
        Samoyed::LockFile l(fileName);
        assert(l.queryState() == Samoyed::LockFile::STATE_UNLOCKED);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_THIS_LOCK);
        assert(strcmp(l.lockingHostName(), hostName) == 0);
        assert(l.lockingProcessId() == getpid());
    }

    snprintf(buffer, sizeof(buffer), "%s %d", hostName, getpid());
    if (g_file_set_contents(fileName, buffer, -1, NULL))
    {
        Samoyed::LockFile l(fileName);
        assert(l.queryState() ==
               Samoyed::LockFile::STATE_LOCKED_BY_THIS_PROCESS);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_THIS_PROCESS);
        assert(strcmp(l.lockingHostName(), hostName) == 0);
        assert(l.lockingProcessId() == getpid());
    }

    snprintf(buffer, sizeof(buffer), "%s 0", hostName);
    if (g_file_set_contents(fileName, buffer, -1, NULL))
    {
        Samoyed::LockFile l(fileName);
        assert(l.queryState() ==
               Samoyed::LockFile::STATE_LOCKED_BY_ANOTHER_PROCESS);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_ANOTHER_PROCESS);
        assert(strcmp(l.lockingHostName(), hostName) == 0);
        assert(l.lockingProcessId() == 0);
    }

    snprintf(buffer, sizeof(buffer), "%s 1000000", hostName);
    if (g_file_set_contents(fileName, buffer, -1, NULL))
    {
        Samoyed::LockFile l(fileName);
        assert(l.queryState() == Samoyed::LockFile::STATE_UNLOCKED);
        assert(l.lock() == Samoyed::LockFile::STATE_LOCKED_BY_THIS_LOCK);
        assert(strcmp(l.lockingHostName(), hostName) == 0);
        assert(l.lockingProcessId() == getpid());
    }

    return 0;
}

#endif
