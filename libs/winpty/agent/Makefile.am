bin_PROGRAMS = winpty-agent

winpty_agent_SOURCES = \
    Agent.cc \
    AgentAssert.cc \
    ConsoleInput.cc \
    Coord.cc \
    EventLoop.cc \
    main.cc \
    NamedPipe.cc \
    SmallRect.cc \
    Terminal.cc \
    Win32Console.cc \
    Agent.h \
    AgentAssert.h \
    ConsoleInput.h \
    Coord.h \
    DsrSender.h \
    EventLoop.h \
    NamedPipe.h \
    SmallRect.h \
    Terminal.h \
    Win32Console.h

winpty_agent_CPPFLAGS = \
    -DUNICODE \
    -D_UNICODE \
    -D_WIN32_WINNT=0x0501

winpty_agent_CXXFLAGS = \
    -fno-exceptions \
    -fno-rtti

winpty_agent_LDFLAGS = \
    -static-libgcc \
    -static-libstdc++

winpty_agent_LDADD = ../shared/libshared.la
