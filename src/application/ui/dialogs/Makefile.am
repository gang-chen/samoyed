noinst_LTLIBRARIES = libdialogs.la

libdialogs_la_SOURCES = \
    session-chooser-dialog.cpp \
    session-chooser-dialog.hpp

libdialogs_la_CPPFLAGS = \
    -DSMYD_PKGDATADIR=\"$(pkgdatadir)\" \
    -DSMYD_LIBDIR=\"$(libdir)\" \
    -DSMYD_LOCALEDIR=\"$(localedir)\" \
    $(BOOST_CPPFLAGS) \
    $(GTK_CFLAGS)
