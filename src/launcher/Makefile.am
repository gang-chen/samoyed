bin_PROGRAMS = samoyed

samoyed_SOURCES = main.cpp

samoyed_LDADD = \
	libsamoyed.la \
	$(BOOST_THREAD_LIB) \
	$(BOOST_REGEX_LIB) \
	$(GTK3_LIBS)

if OS_WIN32
	samoyed_LDFLAGS = -mwindows
endif
