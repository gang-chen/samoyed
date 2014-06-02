bin_PROGRAMS = samoyed

samoyed_SOURCES = main.cpp

samoyed_CPPFLAGS = \
    $(BOOST_CPPFLAGS) \
    $(GTK_CFLAGS)

samoyed_CXXFLAGS = -Werror -Wall

if OS_WIN32
    samoyed_LDFLAGS = -mwindows
endif

samoyed_LDADD = \
    $(SAMOYED_LIBSAMOYED)
