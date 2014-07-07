pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libsearch.la

libsearch_la_SOURCES = \
    search-plugin.cpp \
    search-plugin.hpp

libsearch_la_CPPFLAGS = $(SAMOYED_CPPFLAGS)

libsearch_la_CXXFLAGS = $(SAMOYED_CXXFLAGS)

libsearch_la_LIBADD = $(SAMOYED_PLUGIN_LIBS)

libsearch_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)
