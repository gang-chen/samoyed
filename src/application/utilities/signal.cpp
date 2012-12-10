// Signal handling.
// Copyright (C) 2011 Gang Chen.

/*
UNIT TEST BUILD
g++ signal.cpp -DSMYD_SIGNAL_UNIT_TEST -Werror -Wall -o signal
*/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "signal.hpp"
#include <vector>
#ifdef OS_WIN32
# define _WIN32_WINNT 0x0500
# include <windows.h>
#else
# include <signal.h>
#endif
#ifdef SMYD_SIGNAL_UNIT_TEST
# include <stdio.h>
# include <sys/types.h>
# include <unistd.h>
#endif

namespace
{

std::vector<Samoyed::Signal::SignalHandler> crashHandlers;
std::vector<Samoyed::Signal::SignalHandler> terminationHandlers;

#ifndef OS_WIN32
struct sigaction savedFpeSignalAction;
struct sigaction savedIllSignalAction;
struct sigaction savedSegvSignalAction;
struct sigaction savedBusSignalAction;
struct sigaction savedAbrtSignalAction;
struct sigaction savedTrapSignalAction;
struct sigaction savedSysSignalAction;
struct sigaction savedTermSignalAction;
volatile bool crashing = false;
volatile bool terminating = false;
#endif

#ifdef OS_WIN32
LONG WINAPI onCrashed(PEXCEPTION_POINTERS exceptionRecord)
{
    for (std::vector<Samoyed::Signal::SignalHandler>::const_iterator i =
            crashHandlers.begin();
         i != crashHandlers.end();
         ++i)
        (*i)(exceptionPointers->ExceptionRecord->ExceptionCode);
    return EXCEPTION_CONTINUE_SEARCH;
}
#else
void onCrashed(int signalNumber)
{
    if (crashing)
        raise(signalNumber);
    crashing = true;
    switch (signalNumber)
    {
    case SIGFPE:
        sigaction(SIGFPE, &savedFpeSignalAction, 0);
        break;
    case SIGILL:
        sigaction(SIGILL, &savedIllSignalAction, 0);
        break;
    case SIGSEGV:
        sigaction(SIGSEGV, &savedSegvSignalAction, 0);
        break;
    case SIGBUS:
        sigaction(SIGBUS, &savedBusSignalAction, 0);
        break;
    case SIGABRT:
        sigaction(SIGABRT, &savedAbrtSignalAction, 0);
        break;
    case SIGTRAP:
        sigaction(SIGTRAP, &savedTrapSignalAction, 0);
        break;
    case SIGSYS:
        sigaction(SIGSYS, &savedSysSignalAction, 0);
        break;
    }
    for (std::vector<Samoyed::Signal::SignalHandler>::const_iterator i =
             crashHandlers.begin();
         i != crashHandlers.end();
         ++i)
        (*i)(signalNumber);
    raise(signalNumber);
}
#endif

#ifndef OS_WIN32
void onTerminate(int signalNumber)
{
    if (terminating)
        raise(signalNumber);
    terminating = true;
    sigaction(SIGTERM, &savedTermSignalAction, 0);
    for (std::vector<Samoyed::Signal::SignalHandler>::const_iterator i =
             terminationHandlers.begin();
         i != terminationHandlers.end();
         ++i)
        (*i)(signalNumber);
    raise(signalNumber);
}
#endif

}

namespace Samoyed
{

void Signal::registerCrashHandler(SignalHandler handler)
{
    if (crashHandlers.empty())
    {
#ifdef OS_WIN32
        AddVectoredExceptionHandler(1, onCrashed);
#else
        // Catch SIGFPE, SIGILL, SIGSEGV, SIGBUS, SIGABRT, SIGTRAP and SIGSYS.
        struct sigaction newAction;
        newAction.sa_handler = onCrashed;
        sigemptyset(&newAction.sa_mask);
        newAction.sa_flags = 0;
        sigaction(SIGFPE, &newAction, &savedFpeSignalAction);
        sigaction(SIGILL, &newAction, &savedIllSignalAction);
        sigaction(SIGSEGV, &newAction, &savedSegvSignalAction);
        sigaction(SIGBUS, &newAction, &savedBusSignalAction);
        sigaction(SIGABRT, &newAction, &savedAbrtSignalAction);
        sigaction(SIGTRAP, &newAction, &savedTrapSignalAction);
        sigaction(SIGSYS, &newAction, &savedSysSignalAction);
#endif
    }
    crashHandlers.push_back(handler);
}

void Signal::registerTerminationHandler(SignalHandler handler)
{
#ifndef OS_WIN32
    if (terminationHandlers.empty())
    {
        // Catch SIGTERM.
        sigaction(SIGTERM, 0, &savedTermSignalAction);
        if (savedTermSignalAction.sa_handler != SIG_IGN)
        {
            struct sigaction newAction;
            newAction.sa_handler = onTerminate;
            sigemptyset(&newAction.sa_mask);
            newAction.sa_flags = 0;
            sigaction(SIGTERM, &newAction, 0);
        }
    }
    terminationHandlers.push_back(handler);
#endif
}

}

#ifdef SMYD_SIGNAL_UNIT_TEST

namespace
{

int i = 0;

void myOnCrashed(int sig)
{
    printf("Crashed at %d!\n", i);
    if (sig == SIGSEGV)
        printf("Segmentation fault!\n");
}

void myOnKilled(int sig)
{
    printf("Killed at %d!\n", i);
}

}

int main()
{
    int* p = 0;
    int a;
    Samoyed::Signal::registerCrashHandler(myOnCrashed);
    Samoyed::Signal::registerTerminationHandler(myOnKilled);
    for ( ; i < 100; ++i)
    {
        printf("Type any key:\n");
        scanf("%d", &a);
        if (a == 3)
            *p = 3;
#ifndef OS_WIN32
        if (a == 8)
            kill(getpid(), SIGTERM);
#endif
    }
    return 0;
}

#endif // #ifdef SMYD_SIGNAL_UNIT_TEST
