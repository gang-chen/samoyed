bin_PROGRAMS = samoyed

samoyed_SOURCES = main.cpp

samoyed_CPPFLAGS = \
    $(BOOST_CPPFLAGS) \
    $(GTK_CFLAGS)

samoyed_CXXFLAGS = $(SAMOYED_CXXFLAGS)

if OS_WINDOWS
    samoyed_LDFLAGS = -mwindows
endif

samoyed_LDADD = \
    $(SAMOYED_LIBSAMOYED)
