SUBDIRS = resources ui utilities

lib_LTLIBRARIES = libsamoyed.la

DEFS = \
    -DPKGDATADIR=\"$(pkgdatadir)\" \
    -DLIBDIR=\"$(libdir)\" \
    -DLOCALEDIR=\"$(localedir)\" \
    @DEFS@

libsamoyed_la_SOURCES = \
    application.cpp \
    file-type-registry.cpp \
    session.cpp \
    application.hpp \
    file-type-registry.hpp \
    session.hpp

libsamoyed_la_LIBADD = \
    resources/libresources.la \
    ui/libui.la \
    ui/dialogs/libdialogs.la \
    utilities/libutilities.la
