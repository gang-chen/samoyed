pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libfinder.la

libfinder_la_SOURCES = \
    finder-plugin.cpp \
    finder-plugin.hpp

libfinder_la_CPPFLAGS = $(SAMOYED_CPPFLAGS)

libfinder_la_CXXFLAGS = $(SAMOYED_CXXFLAGS)

libfinder_la_LIBADD = $(SAMOYED_PLUGIN_LIBS)

libfinder_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)
