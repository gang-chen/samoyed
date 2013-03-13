SUBDIRS = resources ui utilities

lib_LTLIBRARIES = libsamoyed.la

libsamoyed_la_SOURCES = \
    application.cpp \
    application.hpp

libsamoyed_la_CPPFLAGS = \
    -DSAMOYED_PKGDATADIR=\"$(pkgdatadir)\" \
    -DSAMOYED_PKGLIBDIR=\"$(pkglibdir)\" \
    -DSAMOYED_LOCALEDIR=\"$(localedir)\" \
    $(BOOST_CPPFLAGS) \
    $(GTK_CFLAGS)

libsamoyed_la_CXXFLAGS = -Werror -Wall

libsamoyed_la_LDFLAGS = $(BOOST_LDFLAGS)

libsamoyed_la_LIBADD = \
    resources/libresources.la \
    ui/libui.la \
    ui/bars/libbars.la \
    ui/dialogs/libdialogs.la \
    utilities/libutilities.la \
    $(BOOST_THREAD_LIB) \
    $(BOOST_REGEX_LIB) \
    $(GTK_LIBS) \
    $(XML_LIBS)
