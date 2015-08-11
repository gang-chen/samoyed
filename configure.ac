AC_PREREQ([2.63])

m4_define([samoyed_major_version], [0])
m4_define([samoyed_minor_version], [0])
m4_define([samoyed_micro_version], [0])
m4_define([samoyed_version],
          [samoyed_major_version.samoyed_minor_version.samoyed_micro_version])

AC_INIT([Samoyed], [samoyed_version])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([launcher/src/launcher.cpp])
AC_CONFIG_MACRO_DIR([m4])

AC_DEFINE([SAMOYED_MAJOR_VERSION], [samoyed_major_version], [Samoyed major version])
AC_SUBST([SAMOYED_MAJOR_VERSION], [samoyed_major_version])
AC_DEFINE([SAMOYED_MINOR_VERSION], [samoyed_minor_version], [Samoyed minor version])
AC_SUBST([SAMOYED_MINOR_VERSION], [samoyed_minor_version])
AC_DEFINE([SAMOYED_MICRO_VERSION], [samoyed_micro_version], [Samoyed micro version])
AC_SUBST([SAMOYED_MICRO_VERSION], [samoyed_micro_version])

AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.11])
AM_MAINTAINER_MODE
AM_SILENT_RULES([yes])

LT_PREREQ([2.2])
LT_INIT([disable-static win32-dll])

AC_LANG([C++])
AC_PROG_CXX
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG([0.26])

AC_CANONICAL_HOST
os_windows=no
case "$host_os" in
  mingw*)
    os_windows=yes
    ;;
esac
if test "x$os_windows" = "xyes"; then
  AC_DEFINE([OS_WINDOWS], [], [Define if the host OS is Windows.])
fi
AM_CONDITIONAL([OS_WINDOWS], [test "x$os_windows" = "xyes"])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18])
IT_PROG_INTLTOOL([0.40])
AC_DEFINE([GETTEXT_PACKAGE], [PACKAGE_TARNAME], [Translation domain])
AC_SUBST([GETTEXT_PACKAGE], [PACKAGE_TARNAME])

AC_CHECK_HEADERS(sys/select.h sys/syslimits.h sys/termios.h sys/un.h sys/wait.h stropts.h termios.h util.h wchar.h)
AC_CHECK_FUNCS([getpgid round pread pwrite fwrite_unlocked])

AX_BOOST_BASE([1.42])
AX_BOOST_CHRONO
AX_BOOST_REGEX
AX_BOOST_SYSTEM
AX_BOOST_THREAD

PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0.0])
PKG_CHECK_MODULES([GMODULE], [gmodule-2.0 >= 2.32.0])
PKG_CHECK_MODULES([XML], [libxml-2.0 >= 2.6.0])
PKG_CHECK_MODULES([GTKSOURCEVIEW], [gtksourceview-3.0 >= 3.0.0])
if test "x$os_windows" = "xno"; then
  PKG_CHECK_MODULES([VTE], [vte-2.90 >= 0.32.0])
fi

AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])
AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])
AC_PATH_PROG([GLIB_MKENUMS], [glib-mkenums])

AC_CHECK_LIB([clang], [clang_createIndex])
AC_CHECK_HEADERS([clang-c/Index.h])

AX_PATH_BDB([6])

SAMOYED_CPPFLAGS="-I\$(top_srcdir)/application/src -I\$(top_srcdir)/application/libs $BOOST_CPPFLAGS $GTK_CFLAGS $GMODULE_CFLAGS $XML_CFLAGS $GTKSOURCEVIEW_CFLAGS $BDB_CPPFLAGS"
AC_SUBST([SAMOYED_CPPFLAGS])

if test "x$os_windows" = "xyes"; then
  SAMOYED_CXXFLAGS=""
else
  SAMOYED_CXXFLAGS="-Werror -Wall"
fi
AC_SUBST(SAMOYED_CXXFLAGS)

SAMOYED_LIBS="$BOOST_CHRONO_LIB $BOOST_REGEX_LIB $BOOST_SYSTEM_LIB $BOOST_THREAD_LIB $GTK_LIBS $GMODULE_LIBS $XML_LIBS $GTKSOURCEVIEW_LIBS $BDB_LIBS"
AC_SUBST([SAMOYED_LIBS])

SAMOYED_LDFLAGS="$BOOST_LDFLAGS $BDB_LDFLAGS -no-undefined -avoid-version"
AC_SUBST([SAMOYED_LDFLAGS])

SAMOYED_LIBSAMOYED="\$(top_builddir)/application/src/libsamoyed.la"
AC_SUBST([SAMOYED_LIBSAMOYED])

SAMOYED_PLUGIN_LIBS="$SAMOYED_LIBSAMOYED $SAMOYED_LIBS"
AC_SUBST([SAMOYED_PLUGIN_LIBS])

SAMOYED_PLUGIN_LDFLAGS="$SAMOYED_LDFLAGS -module"
AC_SUBST([SAMOYED_PLUGIN_LDFLAGS])

AC_CONFIG_FILES([
Makefile
application/Makefile
application/data/Makefile
application/data/icons/Makefile
application/data/icons/16x16/Makefile
application/data/icons/16x16/apps/Makefile
application/data/icons/32x32/Makefile
application/data/icons/32x32/apps/Makefile
application/data/icons/48x48/Makefile
application/data/icons/48x48/apps/Makefile
application/data/ui/Makefile
application/libs/Makefile
application/libs/boost/Makefile
application/libs/boost/spirit/Makefile
application/libs/boost/spirit/home/Makefile
application/libs/boost/spirit/home/support/Makefile
application/libs/boost/spirit/home/support/detail/Makefile
application/libs/boost/threadpool/Makefile
application/libs/boost/threadpool/detail/Makefile
application/src/Makefile
application/src/build-system/Makefile
application/src/editors/Makefile
application/src/parsers/Makefile
application/src/plugin/Makefile
application/src/project/Makefile
application/src/session/Makefile
application/src/utilities/Makefile
application/src/widget/Makefile
application/src/window/Makefile
launcher/Makefile
launcher/src/Makefile
plugins/Makefile
plugins/file-browser/Makefile
plugins/file-browser/data/Makefile
plugins/file-browser/libs/Makefile
plugins/file-browser/libs/gedit-file-browser/Makefile
plugins/file-browser/src/Makefile
plugins/finder/Makefile
plugins/finder/data/Makefile
plugins/finder/data/ui/Makefile
plugins/finder/src/Makefile
plugins/terminal/Makefile
plugins/terminal/data/Makefile
plugins/terminal/libs/Makefile
plugins/terminal/libs/vte/Makefile
plugins/terminal/libs/winpty/Makefile
plugins/terminal/libs/winpty/agent/Makefile
plugins/terminal/libs/winpty/include/Makefile
plugins/terminal/libs/winpty/libwinpty/Makefile
plugins/terminal/libs/winpty/shared/Makefile
plugins/terminal/src/Makefile
plugins/text-file-recoverer/Makefile
plugins/text-file-recoverer/data/Makefile
plugins/text-file-recoverer/src/Makefile
po/Makefile.in
tools/Makefile
tools/windows-installer/Makefile
])
AC_OUTPUT
