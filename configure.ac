AC_PREREQ([2.63])

m4_define([samoyed_major_version], [0])
m4_define([samoyed_minor_version], [0])
m4_define([samoyed_micro_version], [0])
m4_define([samoyed_version],
          [samoyed_major_version.samoyed_minor_version.samoyed_micro_version])

AC_INIT([Samoyed], [samoyed_version])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/launcher/main.cpp])
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

AC_CHECK_LIB([sqlite3], [sqlite3_open])
AC_CHECK_HEADERS([sqlite3.h])

SAMOYED_CPPFLAGS="-I\$(top_srcdir)/src/application -I\$(top_srcdir)/libs $BOOST_CPPFLAGS $GTK_CFLAGS $GMODULE_CFLAGS $XML_CFLAGS $GTKSOURCEVIEW_CFLAGS"
AC_SUBST([SAMOYED_CPPFLAGS])

if test "x$os_windows" = "xyes"; then
  SAMOYED_CXXFLAGS=""
else
  SAMOYED_CXXFLAGS="-Werror -Wall"
fi
AC_SUBST(SAMOYED_CXXFLAGS)

SAMOYED_LIBS="$BOOST_CHRONO_LIB $BOOST_REGEX_LIB $BOOST_SYSTEM_LIB $BOOST_THREAD_LIB $GTK_LIBS $GMODULE_LIBS $XML_LIBS $GTKSOURCEVIEW_LIBS"
AC_SUBST([SAMOYED_LIBS])

SAMOYED_LDFLAGS="$BOOST_LDFLAGS $BDB_LDFLAGS -no-undefined -avoid-version"
AC_SUBST([SAMOYED_LDFLAGS])

SAMOYED_LIBSAMOYED="\$(top_builddir)/src/application/libsamoyed.la"
AC_SUBST([SAMOYED_LIBSAMOYED])

SAMOYED_PLUGIN_LIBS="$SAMOYED_LIBSAMOYED $SAMOYED_LIBS"
AC_SUBST([SAMOYED_PLUGIN_LIBS])

SAMOYED_PLUGIN_LDFLAGS="$SAMOYED_LDFLAGS -module"
AC_SUBST([SAMOYED_PLUGIN_LDFLAGS])

AC_CONFIG_FILES([
Makefile
data/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/16x16/apps/Makefile
data/icons/32x32/Makefile
data/icons/32x32/apps/Makefile
data/icons/48x48/Makefile
data/icons/48x48/apps/Makefile
data/plugins/Makefile
data/plugins/file-browser/Makefile
data/plugins/finder/Makefile
data/plugins/terminal/Makefile
data/plugins/text-file-recoverer/Makefile
libs/Makefile
libs/boost/Makefile
libs/boost/spirit/Makefile
libs/boost/spirit/home/Makefile
libs/boost/spirit/home/support/Makefile
libs/boost/spirit/home/support/detail/Makefile
libs/boost/threadpool/Makefile
libs/boost/threadpool/detail/Makefile
libs/gedit-file-browser/Makefile
libs/vte/Makefile
libs/winpty/Makefile
libs/winpty/agent/Makefile
libs/winpty/include/Makefile
libs/winpty/libwinpty/Makefile
libs/winpty/shared/Makefile
po/Makefile.in
src/Makefile
src/application/Makefile
src/application/editors/Makefile
src/application/parsers/Makefile
src/application/plugin/Makefile
src/application/project/Makefile
src/application/session/Makefile
src/application/utilities/Makefile
src/application/widget/Makefile
src/application/window/Makefile
src/launcher/Makefile
src/plugins/Makefile
src/plugins/file-browser/Makefile
src/plugins/finder/Makefile
src/plugins/terminal/Makefile
src/plugins/text-file-recoverer/Makefile
tools/Makefile
tools/windows-installer/Makefile
])
AC_OUTPUT
