#!/bin/sh
# Run this to generate all the initial makefiles, etc.

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

olddir=`pwd`
cd "$srcdir"

autopoint --force || exit $?
AUTOPOINT='intltoolize --automake --copy' autoreconf --force --install || exit $?
rm po/Makevars.template

cd "$olddir"
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
