bin_PROGRAMS = samoyed

samoyed_SOURCES = main.cpp

samoyed_CXXFLAGS = -Werror -Wall

if OS_WIN32
    samoyed_LDFLAGS = -mwindows
endif

samoyed_LDADD = \
    libsamoyed.la
