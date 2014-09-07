pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libterminal.la

libterminal_la_SOURCES = \
    terminal.cpp \
    terminal-plugin.cpp \
    terminal-view.cpp \
    terminal-view-extension.cpp \
    terminal.hpp \
    terminal-plugin.hpp \
    terminal-view.hpp \
    terminal-view-extension.hpp

libterminal_la_CPPFLAGS = $(SAMOYED_CPPFLAGS)
if !OS_WIN32
    libterminal_la_CPPFLAGS += $(VTE_CFLAGS)
endif

libterminal_la_CXXFLAGS = $(SAMOYED_CXXFLAGS)

libterminal_la_LIBADD = $(SAMOYED_PLUGIN_LIBS)
if !OS_WIN32
    libterminal_la_LIBADD += $(VTE_LIBS)
endif

libterminal_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)
