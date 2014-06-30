pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libtextfilerecoverer.la

libtextfilerecoverer_la_SOURCES = \
    preferences-extension.cpp \
    text-edit.cpp \
    text-edit-saver.cpp \
    text-file-observer-extension.cpp \
    text-file-recoverer.cpp \
    text-file-recoverer-extension.cpp \
    text-file-recoverer-plugin.cpp \
    preferences-extension.hpp \
    text-edit.hpp \
    text-edit-saver.hpp \
    text-file-observer-extension.hpp \
    text-file-recoverer.hpp \
    text-file-recoverer-plugin.hpp

libtextfilerecoverer_la_CPPFLAGS = $(SAMOYED_CPPFLAGS)

libtextfilerecoverer_la_CXXFLAGS = $(SAMOYED_CXXFLAGS)

libtextfilerecoverer_la_LIBADD = $(SAMOYED_PLUGIN_LIBS)

libtextfilerecoverer_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)
