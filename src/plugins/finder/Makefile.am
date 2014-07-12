pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libfinder.la

libfinder_la_SOURCES = \
    find-text-action-extension.cpp \
    finder-plugin.cpp \
    text-finder-bar.cpp \
    find-text-action-extension.hpp \
    finder-plugin.hpp \
    text-finder-bar.hpp

libfinder_la_CPPFLAGS = $(SAMOYED_CPPFLAGS)

libfinder_la_CXXFLAGS = $(SAMOYED_CXXFLAGS)

libfinder_la_LIBADD = $(SAMOYED_PLUGIN_LIBS)

libfinder_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)
