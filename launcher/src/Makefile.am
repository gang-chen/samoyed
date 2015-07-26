bin_PROGRAMS = samoyed

samoyed_SOURCES = launcher.cpp

samoyed_CPPFLAGS = $(SAMOYED_CPPFLAGS)

samoyed_CXXFLAGS = $(SAMOYED_CXXFLAGS)

if OS_WINDOWS
    samoyed_LDFLAGS = -mwindows
endif

samoyed_LDADD = \
    $(SAMOYED_LIBSAMOYED)
