pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libterminal.la

libterminal_la_SOURCES = \
    terminal-plugin.cpp \
    terminal-plugin.hpp

libterminal_la_CPPFLAGS = \
    $(GTK_CFLAGS) \
    $(VTE_CFLAGS)

libterminal_la_CXXFLAGS = -Werror -Wall

libterminal_la_LIBADD = \
    $(GTK_LIBS) \
    $(VTE_LIBS) \
    $(SAMOYED_LIBS)

libterminal_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)

terminaldatadir = $(pkgdatadir)/plugins/terminal

terminaldata_DATA = plugin.xml
