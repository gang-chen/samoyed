SUBDIRS = resources ui utilities

pkglib_LTLIBRARIES = libsamoyed.la

libsamoyed_la_SOURCES = \
    application.cpp \
    application.hpp

libsamoyed_la_CPPFLAGS = \
    $(SAMOYED_CPPFLAGS) \
    -DSAMOYED_PKGDATADIR=\"$(pkgdatadir)\" \
    -DSAMOYED_PKGLIBDIR=\"$(pkglibdir)\" \
    -DSAMOYED_LOCALEDIR=\"$(localedir)\"

libsamoyed_la_CXXFLAGS = $(SAMOYED_CXXFLAGS)

libsamoyed_la_LDFLAGS = \
    $(SAMOYED_LDFLAGS) \
    -avoid-version

libsamoyed_la_LIBADD = \
    resources/libresources.la \
    ui/libui.la \
    ui/bars/libbars.la \
    ui/dialogs/libdialogs.la \
    utilities/libutilities.la \
    $(SAMOYED_LIBS)
