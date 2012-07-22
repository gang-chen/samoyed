// Signal handling.
// Copyright (C) 2011 Gang Chen.

#ifndef SMYD_SIGNAL_HPP
#define SMYD_SIGNAL_HPP

namespace Samoyed
{

class Signal
{
public:
	typedef void (*SignalHandler)(int signalNumber);

	static void registerCrashHandler(SignalHandler handler);
	static void registerTerminationHandler(SignalHandler handler);
};

}

#endif
