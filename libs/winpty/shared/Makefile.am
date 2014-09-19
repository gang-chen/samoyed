noinst_LTLIBRARIES = libshared.la

libshared_la_SOURCES = \
    DebugClient.cc \
    AgentMsg.h \
    Buffer.h \
    c99_snprintf.h \
    DebugClient.h

libshared_la_CPPFLAGS = \
    -DUNICODE \
    -D_UNICODE \
    -D_WIN32_WINNT=0x0501

libshared_la_CXXFLAGS = \
    -fno-exceptions \
    -fno-rtti
