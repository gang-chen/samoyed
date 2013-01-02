AC_PREREQ([2.65])

m4_define(samoyed_major_version, 0)
m4_define(samoyed_minor_version, 0)
m4_define(samoyed_micro_version, 0)
m4_define(samoyed_version,
          samoyed_major_version.samoyed_minor_version.samoyed_micro_version)

AC_INIT([Samoyed], [samoyed_version])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src/launcher/main.cpp])
AC_CONFIG_MACRO_DIR([m4])

AC_DEFINE(SAMOYED_MAJOR_VERSION, samoyed_major_version, [Samoyed major version])
AC_SUBST(SAMOYED_MAJOR_VERSION, samoyed_major_version)
AC_DEFINE(SAMOYED_MINOR_VERSION, samoyed_minor_version, [Samoyed minor version])
AC_SUBST(SAMOYED_MINOR_VERSION, samoyed_minor_version)
AC_DEFINE(SAMOYED_MICRO_VERSION, samoyed_micro_version, [Samoyed micro version])
AC_SUBST(SAMOYED_MICRO_VERSION, samoyed_micro_version)
AC_DEFINE(SAMOYED_VERSION, samoyed_version, [Samoyed version])
AC_SUBST(SAMOYED_VERSION, samoyed_version)

AC_USE_SYSTEM_EXTENSIONS

AM_INIT_AUTOMAKE([1.11 silent-rules])
AM_MAINTAINER_MODE

LT_PREREQ([2.2])
LT_INIT([disable-static])

AC_LANG([C++])
AC_PROG_CXX
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

AC_CANONICAL_HOST
os_win32=no
case $host_os in
  *-*-mingw*)
    os_win32=yes
    ;;
esac
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18])
IT_PROG_INTLTOOL([0.40])
AC_SUBST([GETTEXT_PACKAGE], [samoyed])
AC_DEFINE([GETTEXT_PACKAGE], [PACKAGE_TARNAME], [Translation domain])

AX_BOOST_BASE([1.42])
AX_BOOST_THREAD
AX_BOOST_REGEX

PKG_CHECK_MODULES([GTK], [gtk+-3.0 >= 3.0.0])

AC_CONFIG_FILES([
Makefile
data/Makefile
po/Makefile.in
src/Makefile
src/application/Makefile
src/application/resources/Makefile
src/application/ui/Makefile
src/application/ui/dialogs/Makefile
src/application/utilities/Makefile
src/launcher/Makefile
])
AC_OUTPUT
