pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libtextfilerecoverer.la

libtextfilerecoverer_la_SOURCES = \
    text-file-recoverer-plugin.cpp \
    text-file-recoverer-plugin.hpp

libtextfilerecoverer_la_CPPFLAGS = $(SAMOYED_CPPFLAGS)

libtextfilerecoverer_la_CXXFLAGS = $(SAMOYED_CXXFLAGS)

libtextfilerecoverer_la_LIBADD = $(SAMOYED_PLUGIN_LIBS)

libtextfilerecoverer_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)
