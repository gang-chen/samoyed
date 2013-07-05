pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libterminal.la

libterminal_la_SOURCES = \
    terminal-plugin.cpp \
    terminal-view.cpp \
    terminal-view-extension.cpp \
    terminal-plugin.hpp \
    terminal-view.hpp \
    terminal-view-extension.hpp

libterminal_la_CPPFLAGS = \
    $(SAMOYED_CPPFLAGS) \
    $(VTE_CFLAGS)

libterminal_la_CXXFLAGS = $(SAMOYED_CXXFLAGS)

libterminal_la_LIBADD = \
    $(SAMOYED_PLUGIN_LIBS) \
    $(VTE_LIBS)

libterminal_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)
