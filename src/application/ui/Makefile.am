SUBDIRS = bars dialogs

noinst_LTLIBRARIES = libui.la

libui_la_SOURCES = \
    actions.cpp \
    editor-group.cpp \
    editor.cpp \
    file.cpp \
    pane.cpp \
    project-explorer.cpp \
    project.cpp \
    source-editor.cpp \
    source-file.cpp \
    splash-screen.cpp \
    split-pane.cpp \
    window.cpp \
    actions.hpp \
    bar.hpp \
    editor-group.hpp \
    editor.hpp \
    file.hpp \
    pane-base.hpp \
    pane.hpp \
    project-explorer.hpp \
    project.hpp \
    source-editor.hpp \
    source-file.hpp \
    splash-screen.hpp \
    split-pane.hpp \
    window.hpp

libui_la_CPPFLAGS = \
    $(BOOST_CPPFLAGS) \
    $(GTK_CFLAGS)

libui_la_CXXFLAGS = -Werror -Wall
