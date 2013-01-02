SUBDIRS = resources ui utilities

lib_LTLIBRARIES = libsamoyed.la

libsamoyed_la_SOURCES = \
    application.cpp \
    file-type-registry.cpp \
    session.cpp \
    application.hpp \
    file-type-registry.hpp \
    session.hpp

libsamoyed_la_CPPFLAGS = \
    -DSMYD_PKGDATADIR=\"$(pkgdatadir)\" \
    -DSMYD_LIBDIR=\"$(libdir)\" \
    -DSMYD_LOCALEDIR=\"$(localedir)\" \
    $(BOOST_CPPFLAGS) \
    $(GTK_CFLAGS)

libsamoyed_la_LIBADD = \
    resources/libresources.la \
    ui/libui.la \
    ui/dialogs/libdialogs.la \
    utilities/libutilities.la \
    $(BOOST_THREAD_LIB) \
    $(BOOST_REGEX_LIB) \
    $(GTK_LIBS)
