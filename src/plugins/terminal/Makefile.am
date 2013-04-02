plugindir = $(SAMOYED_PLUGINS_LIBS_DIR)

plugin_LTLIBRARIES = libterminal.la

libterminal_la_SOURCES =

libterminal_la_CPPFLAGS = \
    $(GTK_CFLAGS) \
    $(VTE_CFLAGS)

libterminal_la_CXXFLAGS = -Werror -Wall

libterminal_la_LIBADD = \
    $(GTK_LIBS) \
    $(VTE_LIBS) \
    $(SAMOYED_LIBS)
