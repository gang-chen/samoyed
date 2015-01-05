pluginlibdir = $(pkglibdir)/plugins

pluginlib_LTLIBRARIES = libfilebrowser.la

libfilebrowser_la_SOURCES = \
    file-browser-histories-extension.cpp \
    file-browser-plugin.cpp \
    file-browser-view.cpp \
    file-browser-view-extension.cpp \
    file-browser-histories-extension.hpp \
    file-browser-plugin.hpp \
    file-browser-view.hpp \
    file-browser-view-extension.hpp

libfilebrowser_la_CPPFLAGS = $(SAMOYED_CPPFLAGS)

libfilebrowser_la_CXXFLAGS = $(SAMOYED_CXXFLAGS)

libfilebrowser_la_LIBADD = \
    ../../../libs/gedit-file-browser/libgeditfilebrowser.la \
    $(SAMOYED_PLUGIN_LIBS)

libfilebrowser_la_LDFLAGS = $(SAMOYED_PLUGIN_LDFLAGS)
