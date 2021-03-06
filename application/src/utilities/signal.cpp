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
#include <list>
#ifdef OS_WINDOWS
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

std::list<Samoyed::Signal::SignalHandler> crashHandlers;
std::list<Samoyed::Signal::SignalHandler> terminationHandlers;

#ifndef OS_WINDOWS

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

#ifdef OS_WINDOWS

LONG CALLBACK onCrashed(PEXCEPTION_POINTERS exceptionPointers)
{
    switch (exceptionPointers->ExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        for (std::list<Samoyed::Signal::SignalHandler>::const_iterator i =
                crashHandlers.begin();
             i != crashHandlers.end();
             ++i)
            (*i)(exceptionPointers->ExceptionRecord->ExceptionCode);
    }
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
    for (std::list<Samoyed::Signal::SignalHandler>::const_iterator i =
             crashHandlers.begin();
         i != crashHandlers.end();
         ++i)
        (*i)(signalNumber);
    raise(signalNumber);
}

void onTerminate(int signalNumber)
{
    if (terminating)
        raise(signalNumber);
    terminating = true;
    sigaction(SIGTERM, &savedTermSignalAction, 0);
    for (std::list<Samoyed::Signal::SignalHandler>::const_iterator i =
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
#ifdef OS_WINDOWS
        AddVectoredExceptionHandler(1, onCrashed);
#else
        // Catch SIGILL, SIGSEGV, SIGBUS, SIGABRT, SIGTRAP and SIGSYS.
        struct sigaction newAction;
        newAction.sa_handler = onCrashed;
        sigemptyset(&newAction.sa_mask);
        newAction.sa_flags = 0;
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
#ifndef OS_WINDOWS
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
#ifdef OS_WINDOWS
    if (sig == EXCEPTION_ACCESS_VIOLATION)
        printf("Segmentation fault!\n");
#else
    if (sig == SIGSEGV)
        printf("Segmentation fault!\n");
#endif
    fflush(stdout);
}

void myOnKilled(int sig)
{
    printf("Killed at %d!\n", i);
}

}

int main()
{
    int *p = 0;
    int a;

    Samoyed::Signal::registerCrashHandler(myOnCrashed);
    Samoyed::Signal::registerTerminationHandler(myOnKilled);
    for (; i < 100; ++i)
    {
        printf("Type an integer:\n"
               "  0 - segmentation fault;\n"
               "  1 - termination (Linux only)\n");
        scanf("%d", &a);
        if (a == 0)
            *p = 0;
#ifndef OS_WINDOWS
        else if (a == 1)
            kill(getpid(), SIGTERM);
#endif
    }
    return 0;
}

#endif // #ifdef SMYD_SIGNAL_UNIT_TEST
