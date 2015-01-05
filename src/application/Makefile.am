SUBDIRS = ui utilities

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
    $(SAMOYED_LDFLAGS)

libsamoyed_la_LIBADD = \
    ui/libui.la \
    ui/bars/libbars.la \
    ui/dialogs/libdialogs.la \
    ui/windows/libwindows.la \
    utilities/libutilities.la \
    $(SAMOYED_LIBS)
