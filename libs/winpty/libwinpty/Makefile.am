noinst_LTLIBRARIES = libwinpty.la

libwinpty_la_SOURCES = winpty.cc

libwinpty_la_CPPFLAGS = \
    -I../include \
    -DUNICODE \
    -D_UNICODE \
    -D_WIN32_WINNT=0x0501 \
    -DWINPTY

libwinpty_la_CXXFLAGS = \
    -fno-exceptions \
    -fno-rtti
