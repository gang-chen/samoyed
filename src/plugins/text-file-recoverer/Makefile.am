pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libtext-file-recoverer.la

libtext_file_recoverer_la_SOURCES = \
    text-file-recoverer-plugin.cpp \
    text-file-recoverer-plugin.hpp

libtext_file_recoverer_la_CPPFLAGS = \
    $(GTK_CFLAGS)

libtext_file_recoverer_la_CXXFLAGS = -Werror -Wall

libtext_file_recoverer_la_LIBADD = \
    $(GTK_LIBS) \
    $(SAMOYED_LIBS)

libtext_file_recoverer_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)

text_file_recoverer_datadir = $(pkgdatadir)/plugins/text-file-recoverer

tedata_DATA = plugin.xml
