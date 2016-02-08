BEGIN {
S["am__EXEEXT_FALSE"]="#"
S["am__EXEEXT_TRUE"]=""
S["LTLIBOBJS"]=""
S["LIBOBJS"]=""
S["SAMOYED_PLUGIN_LDFLAGS"]="-L/mingw64/lib  -no-undefined -avoid-version -module"
S["SAMOYED_PLUGIN_LIBS"]="$(top_builddir)/application/src/libsamoyed.la -lboost_chrono-mt -lboost_regex-mt -lboost_system-mt -lboost_thread-mt -L/mingw64/lib -LC:/building/ms"\
"ys64/mingw64/lib -L/mingw64/lib -LC:/building/msys64/mingw64/lib/../lib -L/mingw64/lib -lgtk-3 -lgdk-3 -lgdi32 -limm32 -lshell32 -lole32 -Wl,-luuid "\
"-lwinmm -ldwmapi -lz -lepoxy -lpangocairo-1.0 -lpangoft2-1.0 -lpangowin32-1.0 -lgdi32 -lusp10 -lpango-1.0 -lm -latk-1.0 -lcairo-gobject -lcairo -lz "\
"-lpixman-1 -lfontconfig -lexpat -lfreetype -liconv -lexpat -lfreetype -lz -lbz2 -lharfbuzz -lpng16 -lz -lgdk_pixbuf-2.0 -lpng16 -lz -lgio-2.0 -lz -l"\
"gmodule-2.0 -pthread -lgobject-2.0 -lffi -lglib-2.0 -lintl -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -lintl  -L/mingw64/lib -lgmodule-2.0 -pthread"\
" -lglib-2.0 -lintl -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -lintl  -L/mingw64/lib -LC:/building/msys32/mingw64/lib -lxml2 -lz -llzma -liconv -lw"\
"s2_32  -L/mingw64/lib -LC:/building/msys32/mingw64/lib -L/mingw64/lib -LC:/building/msys64/mingw64/lib -L/mingw64/lib -LC:/building/msys64/mingw64/l"\
"ib/../lib -L/mingw64/lib -lgtksourceview-3.0 -lxml2 -lz -llzma -liconv -lws2_32 -lgtk-3 -lgdk-3 -lgdi32 -limm32 -lshell32 -lole32 -Wl,-luuid -lwinmm"\
" -ldwmapi -lz -lepoxy -lpangocairo-1.0 -lpangoft2-1.0 -lpangowin32-1.0 -lgdi32 -lusp10 -lpango-1.0 -lm -latk-1.0 -lcairo-gobject -lcairo -lz -lpixma"\
"n-1 -lfontconfig -lexpat -lfreetype -liconv -lexpat -lfreetype -lz -lbz2 -lharfbuzz -lpng16 -lz -lgdk_pixbuf-2.0 -lpng16 -lz -lgio-2.0 -lz -lgmodule"\
"-2.0 -pthread -lgobject-2.0 -lffi -lglib-2.0 -lintl -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -lintl  -L/mingw64/lib -LC:/building/msys64/mingw64/"\
"lib -L/mingw64/lib -LC:/building/msys64/mingw64/lib/../lib -L/mingw64/lib -lgio-2.0 -lz -lgmodule-2.0 -pthread -lgobject-2.0 -lffi -lglib-2.0 -lintl"\
" -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -lintl  -ldb"
S["SAMOYED_LIBSAMOYED"]="$(top_builddir)/application/src/libsamoyed.la"
S["SAMOYED_LDFLAGS"]="-L/mingw64/lib  -no-undefined -avoid-version"
S["SAMOYED_LIBS"]="-lboost_chrono-mt -lboost_regex-mt -lboost_system-mt -lboost_thread-mt -L/mingw64/lib -LC:/building/msys64/mingw64/lib -L/mingw64/lib -LC:/building/"\
"msys64/mingw64/lib/../lib -L/mingw64/lib -lgtk-3 -lgdk-3 -lgdi32 -limm32 -lshell32 -lole32 -Wl,-luuid -lwinmm -ldwmapi -lz -lepoxy -lpangocairo-1.0 "\
"-lpangoft2-1.0 -lpangowin32-1.0 -lgdi32 -lusp10 -lpango-1.0 -lm -latk-1.0 -lcairo-gobject -lcairo -lz -lpixman-1 -lfontconfig -lexpat -lfreetype -li"\
"conv -lexpat -lfreetype -lz -lbz2 -lharfbuzz -lpng16 -lz -lgdk_pixbuf-2.0 -lpng16 -lz -lgio-2.0 -lz -lgmodule-2.0 -pthread -lgobject-2.0 -lffi -lgli"\
"b-2.0 -lintl -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -lintl  -L/mingw64/lib -lgmodule-2.0 -pthread -lglib-2.0 -lintl -pthread -lws2_32 -lole32 -"\
"lwinmm -lshlwapi -lintl  -L/mingw64/lib -LC:/building/msys32/mingw64/lib -lxml2 -lz -llzma -liconv -lws2_32  -L/mingw64/lib -LC:/building/msys32/min"\
"gw64/lib -L/mingw64/lib -LC:/building/msys64/mingw64/lib -L/mingw64/lib -LC:/building/msys64/mingw64/lib/../lib -L/mingw64/lib -lgtksourceview-3.0 -"\
"lxml2 -lz -llzma -liconv -lws2_32 -lgtk-3 -lgdk-3 -lgdi32 -limm32 -lshell32 -lole32 -Wl,-luuid -lwinmm -ldwmapi -lz -lepoxy -lpangocairo-1.0 -lpango"\
"ft2-1.0 -lpangowin32-1.0 -lgdi32 -lusp10 -lpango-1.0 -lm -latk-1.0 -lcairo-gobject -lcairo -lz -lpixman-1 -lfontconfig -lexpat -lfreetype -liconv -l"\
"expat -lfreetype -lz -lbz2 -lharfbuzz -lpng16 -lz -lgdk_pixbuf-2.0 -lpng16 -lz -lgio-2.0 -lz -lgmodule-2.0 -pthread -lgobject-2.0 -lffi -lglib-2.0 -"\
"lintl -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -lintl  -L/mingw64/lib -LC:/building/msys64/mingw64/lib -L/mingw64/lib -LC:/building/msys64/mingw6"\
"4/lib/../lib -L/mingw64/lib -lgio-2.0 -lz -lgmodule-2.0 -pthread -lgobject-2.0 -lffi -lglib-2.0 -lintl -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -"\
"lintl  -ldb"
S["SAMOYED_CXXFLAGS"]=""
S["SAMOYED_CFLAGS"]=""
S["SAMOYED_CPPFLAGS"]="-I$(top_srcdir)/application/src -I$(top_srcdir)/application/libs -pthread -I/mingw64/include -mms-bitfields -pthread -mms-bitfields -I/mingw64/inclu"\
"de/gtk-3.0 -I/mingw64/include/cairo -I/mingw64/include -I/mingw64/include/pango-1.0 -I/mingw64/include/atk-1.0 -I/mingw64/include/cairo -I/mingw64/i"\
"nclude/pixman-1 -I/mingw64/include -I/mingw64/include/freetype2 -I/mingw64/include/libpng16 -I/mingw64/include/harfbuzz -I/mingw64/include/glib-2.0 "\
"-I/mingw64/lib/glib-2.0/include -I/mingw64/include -I/mingw64/include/freetype2 -I/mingw64/include -I/mingw64/include/harfbuzz -I/mingw64/include/li"\
"bpng16 -I/mingw64/include/gdk-pixbuf-2.0 -I/mingw64/include/libpng16 -I/mingw64/include/glib-2.0 -I/mingw64/lib/glib-2.0/include  -pthread -mms-bitf"\
"ields -I/mingw64/include/glib-2.0 -I/mingw64/lib/glib-2.0/include  -I/mingw64/include/libxml2  -mms-bitfields -pthread -mms-bitfields -I/mingw64/inc"\
"lude/gtksourceview-3.0 -I/mingw64/include/libxml2 -I/mingw64/include/gtk-3.0 -I/mingw64/include/cairo -I/mingw64/include -I/mingw64/include/pango-1."\
"0 -I/mingw64/include/atk-1.0 -I/mingw64/include/cairo -I/mingw64/include/pixman-1 -I/mingw64/include -I/mingw64/include/freetype2 -I/mingw64/include"\
"/libpng16 -I/mingw64/include/harfbuzz -I/mingw64/include/glib-2.0 -I/mingw64/lib/glib-2.0/include -I/mingw64/include -I/mingw64/include/freetype2 -I"\
"/mingw64/include -I/mingw64/include/harfbuzz -I/mingw64/include/libpng16 -I/mingw64/include/gdk-pixbuf-2.0 -I/mingw64/include/libpng16 -I/mingw64/in"\
"clude/glib-2.0 -I/mingw64/lib/glib-2.0/include  -pthread -mms-bitfields -I/mingw64/include/gio-win32-2.0/ -I/mingw64/include/glib-2.0 -I/mingw64/lib"\
"/glib-2.0/include  "
S["GLIB_MKENUMS"]="/mingw64/bin/glib-mkenums"
S["GLIB_GENMARSHAL"]="/mingw64/bin/glib-genmarshal"
S["GLIB_COMPILE_RESOURCES"]="/mingw64/bin/glib-compile-resources"
S["VTE_LIBS"]=""
S["VTE_CFLAGS"]=""
S["GIO_UNIX_LIBS"]=""
S["GIO_UNIX_CFLAGS"]=""
S["GIO_WINDOWS_LIBS"]="-L/mingw64/lib -LC:/building/msys64/mingw64/lib -L/mingw64/lib -LC:/building/msys64/mingw64/lib/../lib -L/mingw64/lib -lgio-2.0 -lz -lgmodule-2.0 -p"\
"thread -lgobject-2.0 -lffi -lglib-2.0 -lintl -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -lintl "
S["GIO_WINDOWS_CFLAGS"]="-pthread -mms-bitfields -I/mingw64/include/gio-win32-2.0/ -I/mingw64/include/glib-2.0 -I/mingw64/lib/glib-2.0/include "
S["GTKSOURCEVIEW_LIBS"]="-L/mingw64/lib -LC:/building/msys32/mingw64/lib -L/mingw64/lib -LC:/building/msys64/mingw64/lib -L/mingw64/lib -LC:/building/msys64/mingw64/lib/../l"\
"ib -L/mingw64/lib -lgtksourceview-3.0 -lxml2 -lz -llzma -liconv -lws2_32 -lgtk-3 -lgdk-3 -lgdi32 -limm32 -lshell32 -lole32 -Wl,-luuid -lwinmm -ldwma"\
"pi -lz -lepoxy -lpangocairo-1.0 -lpangoft2-1.0 -lpangowin32-1.0 -lgdi32 -lusp10 -lpango-1.0 -lm -latk-1.0 -lcairo-gobject -lcairo -lz -lpixman-1 -lf"\
"ontconfig -lexpat -lfreetype -liconv -lexpat -lfreetype -lz -lbz2 -lharfbuzz -lpng16 -lz -lgdk_pixbuf-2.0 -lpng16 -lz -lgio-2.0 -lz -lgmodule-2.0 -p"\
"thread -lgobject-2.0 -lffi -lglib-2.0 -lintl -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -lintl "
S["GTKSOURCEVIEW_CFLAGS"]="-mms-bitfields -pthread -mms-bitfields -I/mingw64/include/gtksourceview-3.0 -I/mingw64/include/libxml2 -I/mingw64/include/gtk-3.0 -I/mingw64/include"\
"/cairo -I/mingw64/include -I/mingw64/include/pango-1.0 -I/mingw64/include/atk-1.0 -I/mingw64/include/cairo -I/mingw64/include/pixman-1 -I/mingw64/in"\
"clude -I/mingw64/include/freetype2 -I/mingw64/include/libpng16 -I/mingw64/include/harfbuzz -I/mingw64/include/glib-2.0 -I/mingw64/lib/glib-2.0/inclu"\
"de -I/mingw64/include -I/mingw64/include/freetype2 -I/mingw64/include -I/mingw64/include/harfbuzz -I/mingw64/include/libpng16 -I/mingw64/include/gdk"\
"-pixbuf-2.0 -I/mingw64/include/libpng16 -I/mingw64/include/glib-2.0 -I/mingw64/lib/glib-2.0/include "
S["XML_LIBS"]="-L/mingw64/lib -LC:/building/msys32/mingw64/lib -lxml2 -lz -llzma -liconv -lws2_32 "
S["XML_CFLAGS"]="-I/mingw64/include/libxml2 "
S["GMODULE_LIBS"]="-L/mingw64/lib -lgmodule-2.0 -pthread -lglib-2.0 -lintl -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -lintl "
S["GMODULE_CFLAGS"]="-pthread -mms-bitfields -I/mingw64/include/glib-2.0 -I/mingw64/lib/glib-2.0/include "
S["GTK_LIBS"]="-L/mingw64/lib -LC:/building/msys64/mingw64/lib -L/mingw64/lib -LC:/building/msys64/mingw64/lib/../lib -L/mingw64/lib -lgtk-3 -lgdk-3 -lgdi32 -limm3"\
"2 -lshell32 -lole32 -Wl,-luuid -lwinmm -ldwmapi -lz -lepoxy -lpangocairo-1.0 -lpangoft2-1.0 -lpangowin32-1.0 -lgdi32 -lusp10 -lpango-1.0 -lm -latk-1"\
".0 -lcairo-gobject -lcairo -lz -lpixman-1 -lfontconfig -lexpat -lfreetype -liconv -lexpat -lfreetype -lz -lbz2 -lharfbuzz -lpng16 -lz -lgdk_pixbuf-2"\
".0 -lpng16 -lz -lgio-2.0 -lz -lgmodule-2.0 -pthread -lgobject-2.0 -lffi -lglib-2.0 -lintl -pthread -lws2_32 -lole32 -lwinmm -lshlwapi -lintl "
S["GTK_CFLAGS"]="-mms-bitfields -pthread -mms-bitfields -I/mingw64/include/gtk-3.0 -I/mingw64/include/cairo -I/mingw64/include -I/mingw64/include/pango-1.0 -I/mingw6"\
"4/include/atk-1.0 -I/mingw64/include/cairo -I/mingw64/include/pixman-1 -I/mingw64/include -I/mingw64/include/freetype2 -I/mingw64/include/libpng16 -"\
"I/mingw64/include/harfbuzz -I/mingw64/include/glib-2.0 -I/mingw64/lib/glib-2.0/include -I/mingw64/include -I/mingw64/include/freetype2 -I/mingw64/in"\
"clude -I/mingw64/include/harfbuzz -I/mingw64/include/libpng16 -I/mingw64/include/gdk-pixbuf-2.0 -I/mingw64/include/libpng16 -I/mingw64/include/glib-"\
"2.0 -I/mingw64/lib/glib-2.0/include "
S["BOOST_THREAD_LIB"]="-lboost_thread-mt"
S["BOOST_SYSTEM_LIB"]="-lboost_system-mt"
S["BOOST_REGEX_LIB"]="-lboost_regex-mt"
S["BOOST_CHRONO_LIB"]="-lboost_chrono-mt"
S["BOOST_LDFLAGS"]="-L/mingw64/lib"
S["BOOST_CPPFLAGS"]="-pthread -I/mingw64/include"
S["GETTEXT_PACKAGE"]="PACKAGE_TARNAME"
S["ALL_LINGUAS"]=""
S["INTLTOOL_PERL"]="/usr/bin/perl"
S["INTLTOOL_POLICY_RULE"]="%.policy:    %.policy.in    $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -x -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_SERVICE_RULE"]="%.service: %.service.in   $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_OPT"\
"IONS) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_THEME_RULE"]="%.theme:     %.theme.in     $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_SCHEMAS_RULE"]="%.schemas:   %.schemas.in   $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -s -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_CAVES_RULE"]="%.caves:     %.caves.in     $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_XML_NOMERGE_RULE"]="%.xml:       %.xml.in       $(INTLTOOL_MERGE) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_OPTIONS) -x -u --no-translations $<"\
" $@"
S["INTLTOOL_XML_RULE"]="%.xml:       %.xml.in       $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -x -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_KBD_RULE"]="%.kbd:       %.kbd.in       $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -x -u -m -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_XAM_RULE"]="%.xam:       %.xml.in       $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -x -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_UI_RULE"]="%.ui:        %.ui.in        $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -x -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_SOUNDLIST_RULE"]="%.soundlist: %.soundlist.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_SHEET_RULE"]="%.sheet:     %.sheet.in     $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -x -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_SERVER_RULE"]="%.server:    %.server.in    $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -o -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_PONG_RULE"]="%.pong:      %.pong.in      $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -x -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_OAF_RULE"]="%.oaf:       %.oaf.in       $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -o -p $(top_srcdir)/po $< $@"
S["INTLTOOL_PROP_RULE"]="%.prop:      %.prop.in      $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_KEYS_RULE"]="%.keys:      %.keys.in      $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -k -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_DIRECTORY_RULE"]="%.directory: %.directory.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["INTLTOOL_DESKTOP_RULE"]="%.desktop:   %.desktop.in   $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_V_MERGE)LC_ALL=C $(INTLTOOL_MERGE) $(INTLTOOL_V_MERGE_O"\
"PTIONS) -d -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@"
S["intltool__v_merge_options_0"]="-q"
S["intltool__v_merge_options_"]="$(intltool__v_merge_options_$(AM_DEFAULT_VERBOSITY))"
S["INTLTOOL_V_MERGE_OPTIONS"]="$(intltool__v_merge_options_$(V))"
S["INTLTOOL__v_MERGE_0"]="@echo \"  ITMRG \" $@;"
S["INTLTOOL__v_MERGE_"]="$(INTLTOOL__v_MERGE_$(AM_DEFAULT_VERBOSITY))"
S["INTLTOOL_V_MERGE"]="$(INTLTOOL__v_MERGE_$(V))"
S["INTLTOOL_EXTRACT"]="/usr/bin/intltool-extract"
S["INTLTOOL_MERGE"]="/usr/bin/intltool-merge"
S["INTLTOOL_UPDATE"]="/usr/bin/intltool-update"
S["POSUB"]="po"
S["LTLIBINTL"]="-lintl"
S["LIBINTL"]="-lintl"
S["INTLLIBS"]="-lintl"
S["LTLIBICONV"]="-liconv"
S["LIBICONV"]="-liconv"
S["INTL_MACOSX_LIBS"]=""
S["XGETTEXT_EXTRA_OPTIONS"]=""
S["MSGMERGE"]="/mingw64/bin/msgmerge"
S["XGETTEXT_015"]="/mingw64/bin/xgettext"
S["XGETTEXT"]="/mingw64/bin/xgettext"
S["GMSGFMT_015"]="/mingw64/bin/msgfmt"
S["MSGFMT_015"]="/mingw64/bin/msgfmt"
S["GMSGFMT"]="/mingw64/bin/msgfmt"
S["MSGFMT"]="/mingw64/bin/msgfmt"
S["GETTEXT_MACRO_VERSION"]="0.18"
S["USE_NLS"]="yes"
S["OS_WINDOWS_FALSE"]="#"
S["OS_WINDOWS_TRUE"]=""
S["PKG_CONFIG_LIBDIR"]=""
S["PKG_CONFIG_PATH"]="/mingw64/lib/pkgconfig:/mingw64/share/pkgconfig"
S["PKG_CONFIG"]="/usr/bin/pkg-config"
S["CXXCPP"]="g++ -E"
S["am__fastdepCXX_FALSE"]="#"
S["am__fastdepCXX_TRUE"]=""
S["CXXDEPMODE"]="depmode=gcc3"
S["ac_ct_CXX"]="g++"
S["CXXFLAGS"]="-g -O2"
S["CXX"]="g++"
S["LT_SYS_LIBRARY_PATH"]=""
S["OTOOL64"]=""
S["OTOOL"]=""
S["LIPO"]=""
S["NMEDIT"]=""
S["DSYMUTIL"]=""
S["MANIFEST_TOOL"]=":"
S["RANLIB"]="ranlib"
S["ac_ct_AR"]="ar"
S["AR"]="ar"
S["LN_S"]="cp -pR"
S["NM"]="/mingw64/bin/nm -B"
S["ac_ct_DUMPBIN"]=""
S["DUMPBIN"]=""
S["LD"]="D:/msys64/mingw64/x86_64-w64-mingw32/bin/ld.exe"
S["FGREP"]="/usr/bin/grep -F"
S["SED"]="/usr/bin/sed"
S["host_os"]="mingw64"
S["host_vendor"]="pc"
S["host_cpu"]="x86_64"
S["host"]="x86_64-pc-mingw64"
S["build_os"]="mingw64"
S["build_vendor"]="pc"
S["build_cpu"]="x86_64"
S["build"]="x86_64-pc-mingw64"
S["LIBTOOL"]="$(SHELL) $(top_builddir)/libtool"
S["OBJDUMP"]="objdump"
S["DLLTOOL"]="dlltool"
S["AS"]="as"
S["MAINT"]="#"
S["MAINTAINER_MODE_FALSE"]=""
S["MAINTAINER_MODE_TRUE"]="#"
S["AM_BACKSLASH"]="\\"
S["AM_DEFAULT_VERBOSITY"]="0"
S["AM_DEFAULT_V"]="$(AM_DEFAULT_VERBOSITY)"
S["AM_V"]="$(V)"
S["am__fastdepCC_FALSE"]="#"
S["am__fastdepCC_TRUE"]=""
S["CCDEPMODE"]="depmode=gcc3"
S["am__nodep"]="_no"
S["AMDEPBACKSLASH"]="\\"
S["AMDEP_FALSE"]="#"
S["AMDEP_TRUE"]=""
S["am__quote"]=""
S["am__include"]="include"
S["DEPDIR"]=".deps"
S["am__untar"]="$${TAR-tar} xf -"
S["am__tar"]="$${TAR-tar} chof - \"$$tardir\""
S["AMTAR"]="$${TAR-tar}"
S["am__leading_dot"]="."
S["SET_MAKE"]=""
S["AWK"]="gawk"
S["mkdir_p"]="/usr/bin/mkdir -p"
S["MKDIR_P"]="/usr/bin/mkdir -p"
S["INSTALL_STRIP_PROGRAM"]="$(install_sh) -c -s"
S["STRIP"]="strip"
S["install_sh"]="${SHELL} /home/gangchen/samoyed/install-sh"
S["MAKEINFO"]="${SHELL} /home/gangchen/samoyed/missing makeinfo"
S["AUTOHEADER"]="${SHELL} /home/gangchen/samoyed/missing autoheader"
S["AUTOMAKE"]="${SHELL} /home/gangchen/samoyed/missing automake-1.15"
S["AUTOCONF"]="${SHELL} /home/gangchen/samoyed/missing autoconf"
S["ACLOCAL"]="${SHELL} /home/gangchen/samoyed/missing aclocal-1.15"
S["VERSION"]="0.0.0"
S["PACKAGE"]="samoyed"
S["CYGPATH_W"]="cygpath -w"
S["am__isrc"]=""
S["INSTALL_DATA"]="${INSTALL} -m 644"
S["INSTALL_SCRIPT"]="${INSTALL}"
S["INSTALL_PROGRAM"]="${INSTALL}"
S["EGREP"]="/usr/bin/grep -E"
S["GREP"]="/usr/bin/grep"
S["CPP"]="gcc -E"
S["OBJEXT"]="o"
S["EXEEXT"]=".exe"
S["ac_ct_CC"]="gcc"
S["CPPFLAGS"]=""
S["LDFLAGS"]=""
S["CFLAGS"]="-g -O2"
S["CC"]="gcc"
S["SAMOYED_MICRO_VERSION"]="0"
S["SAMOYED_MINOR_VERSION"]="0"
S["SAMOYED_MAJOR_VERSION"]="0"
S["target_alias"]=""
S["host_alias"]=""
S["build_alias"]=""
S["LIBS"]="-lclang "
S["ECHO_T"]=""
S["ECHO_N"]="-n"
S["ECHO_C"]=""
S["DEFS"]="-DHAVE_CONFIG_H"
S["mandir"]="${datarootdir}/man"
S["localedir"]="${datarootdir}/locale"
S["libdir"]="${exec_prefix}/lib"
S["psdir"]="${docdir}"
S["pdfdir"]="${docdir}"
S["dvidir"]="${docdir}"
S["htmldir"]="${docdir}"
S["infodir"]="${datarootdir}/info"
S["docdir"]="${datarootdir}/doc/${PACKAGE_TARNAME}"
S["oldincludedir"]="/usr/include"
S["includedir"]="${prefix}/include"
S["localstatedir"]="${prefix}/var"
S["sharedstatedir"]="${prefix}/com"
S["sysconfdir"]="${prefix}/etc"
S["datadir"]="${datarootdir}"
S["datarootdir"]="${prefix}/share"
S["libexecdir"]="${exec_prefix}/libexec"
S["sbindir"]="${exec_prefix}/sbin"
S["bindir"]="${exec_prefix}/bin"
S["program_transform_name"]="s,x,x,"
S["prefix"]="/usr/local"
S["exec_prefix"]="${prefix}"
S["PACKAGE_URL"]=""
S["PACKAGE_BUGREPORT"]=""
S["PACKAGE_STRING"]="Samoyed 0.0.0"
S["PACKAGE_VERSION"]="0.0.0"
S["PACKAGE_TARNAME"]="samoyed"
S["PACKAGE_NAME"]="Samoyed"
S["PATH_SEPARATOR"]=":"
S["SHELL"]="/bin/sh"
  for (key in S) S_is_set[key] = 1
  FS = ""

}
{
  line = $ 0
  nfields = split(line, field, "@")
  substed = 0
  len = length(field[1])
  for (i = 2; i < nfields; i++) {
    key = field[i]
    keylen = length(key)
    if (S_is_set[key]) {
      value = S[key]
      line = substr(line, 1, len) "" value "" substr(line, len + keylen + 3)
      len += length(value) + length(field[++i])
      substed = 1
    } else
      len += 1 + keylen
  }

  print line
}

