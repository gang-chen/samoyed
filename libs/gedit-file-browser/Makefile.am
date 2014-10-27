noinst_LTLIBRARIES = libgeditfilebrowser.la

geditfilebrowser_BUILTSOURCES = \
    gedit-file-browser-enum-types.h \
    gedit-file-browser-enum-types.c \
    gedit-file-browser-marshal.h \
    gedit-file-browser-marshal.c \
    gedit-file-browser-resources.c

libgeditfilebrowser_la_NOINST_H_FILES = \
    gedit-file-bookmarks-store.h \
    gedit-file-browser-error.h \
    gedit-file-browser-store.h \
    gedit-file-browser-view.h \
    gedit-file-browser-widget.h \
    gedit-file-browser-utils.h \
    gedit-utils.h

libgeditfilebrowser_la_SOURCES = \
    $(geditfilebrowser_BUILTSOURCES) \
    gedit-file-bookmarks-store.c \
    gedit-file-browser-store.c \
    gedit-file-browser-view.c \
    gedit-file-browser-widget.c \
    gedit-file-browser-utils.c \
    gedit-utils.c \
    $(libgeditfilebrowser_la_NOINST_H_FILES)

libgeditfilebrowser_la_CPPFLAGS = \
    $(GTK_CFLAGS)

GRESDEPS = $(srcdir)/$1 $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir)/$(dir $1) $(srcdir)/$1)

GRESGEN = $(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target="$@" --sourcedir="$(dir $<)" --generate-source "$<"

geditfilebrowser_resources_deps = $(call GRESDEPS,resources/gedit-file-browser.gresource.xml)
gedit-file-browser-resources.c: $(geditfilebrowser_resources_deps)
	$(GRESGEN)

geditfilebrowser_enum_c_template = gedit-file-browser-enum-types.c.template
geditfilebrowser_enum_h_template = gedit-file-browser-enum-types.h.template

gedit-file-browser-enum-types.h: $(geditfilebrowser_enum_h_template) $(libgeditfilebrowser_la_NOINST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN) (cd $(srcdir) &&						\
	 $(GLIB_MKENUMS)							\
		--template $(geditfilebrowser_enum_h_template)			\
		$(libgeditfilebrowser_la_NOINST_H_FILES)) > $@

gedit-file-browser-enum-types.c: $(geditfilebrowser_enum_c_template) $(libgeditfilebrowser_la_NOINST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN) 								\
	(cd $(srcdir) &&							\
	 $(GLIB_MKENUMS)							\
		--template $(geditfilebrowser_enum_c_template) 			\
		$(libgeditfilebrowser_la_NOINST_H_FILES)) > $@

gedit-file-browser-marshal.h: gedit-file-browser-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $< --header --prefix=gedit_file_browser_marshal > $@

gedit-file-browser-marshal.c: gedit-file-browser-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN) echo "#include \"gedit-file-browser-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=gedit_file_browser_marshal >> $@

BUILT_SOURCES = $(geditfilebrowser_BUILTSOURCES)

EXTRA_DIST = \
    $(geditfilebrowser_resources_deps) \
    gedit-file-browser-enum-types.h.template \
    gedit-file-browser-enum-types.c.template \
    gedit-file-browser-marshal.list

CLEANFILES = $(geditfilebrowser_BUILTSOURCES)
