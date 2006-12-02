#!/bin/bash
# taken from GNU Gadu project

ACPATH1="/usr/share/aclocal";
ACPATH2="/opt/gnome2/share/aclocal";
DIE=0

if [ -d $ACPATH1 ]; then
    ACPATH="-I $ACPATH1"
fi

if [ -d $ACPATH2 ]; then
    ACPATH="$ACPATH -I $ACPATH2"
fi

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have autoconf installed to compile $PROJECT."
        echo "Install the appropriate package for your distribution,"
        echo "or get the source tarball at http://ftp.gnu.org/gnu/autoconf/"
        DIE=1
}

if automake-1.9 --version < /dev/null > /dev/null 2>&1 ; then
    AUTOMAKE=automake-1.9
    ACLOCAL=aclocal-1.9
else
    if automake-1.8 --version < /dev/null > /dev/null 2>&1 ; then
	AUTOMAKE=automake-1.8
        ACLOCAL=aclocal-1.8
    else
        echo
        echo "You must have at least automake 1.8.x installed to compile $PROJECT."
        echo "Install the appropriate package for your distribution,"
        echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
	echo "\n"
	echo "trying to automake anyway but YOU WERE WARNED"
	AUTOMAKE=automake
    	ACLOCAL=aclocal
    fi
fi

if test "$DIE" -eq 1; then 
        exit 1
fi

if [ -f configure.in~ ]; then
    mv configure.in~ configure.in
fi

if [ -f Makefile.am~ ]; then
    mv Makefile.am~ Makefile.am
fi

echo "aclocal"
$ACLOCAL $ACPATH -I . || exit 1

echo "automake"
$AUTOMAKE --no-force --copy --add-missing || exit 1

echo "autoconf"
autoconf || exit 1
