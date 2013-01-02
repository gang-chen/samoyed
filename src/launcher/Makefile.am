bin_PROGRAMS = samoyed

samoyed_SOURCES = main.cpp

samoyed_LDFLAGS = $(BOOST_LDFLAGS)

if OS_WIN32
    samoyed_LDFLAGS += -mwindows
endif

samoyed_LDADD = \
    libsamoyed.la \
    $(BOOST_THREAD_LIB) \
    $(BOOST_REGEX_LIB) \
    $(GTK_LIBS)
