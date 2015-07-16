SUBDIRS = editors parsers plugin project session utilities widget window

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
    editors/libeditors.la \
    parsers/libparsers.la \
    plugin/libplugin.la \
    project/libproject.la \
    session/libsession.la \
    utilities/libutilities.la \
    widget/libwidget.la \
    window/libwindow.la \
    $(SAMOYED_LIBS)
