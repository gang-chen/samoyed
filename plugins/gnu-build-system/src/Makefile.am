pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libgnubuildsystem.la

libgnubuildsystem_la_SOURCES = \
    gnu-build-system.cpp \
    gnu-build-system-extension.cpp \
    gnu-build-system-file.cpp \
    gnu-build-system-plugin.cpp \
    gnu-build-system.hpp \
    gnu-build-system-extension.hpp \
    gnu-build-system-file.hpp \
    gnu-build-system-plugin.hpp

libgnubuildsystem_la_CPPFLAGS = $(SAMOYED_CPPFLAGS)

libgnubuildsystem_la_CXXFLAGS = $(SAMOYED_CXXFLAGS)

libgnubuildsystem_la_LIBADD = $(SAMOYED_PLUGIN_LIBS)

libgnubuildsystem_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)
