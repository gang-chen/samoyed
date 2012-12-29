lib_LTLIBRARIES = libsamoyed.la

libsamoyed_la_SOURCES = \
    application.cpp \
    file-type-registry.cpp \
    session.cpp

SUBDIRS = \
    resources \
    ui \
    utilities

DEFS = \
    -DPKGDATADIR=\"$(pkgdatadir)\" \
    -DLIBDIR=\"$(libdir)\" \
    -DLOCALEDIR=\"$(localedir)\" \
    @DEFS@
