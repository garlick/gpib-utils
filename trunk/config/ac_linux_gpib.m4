##*****************************************************************************
### $Id: ac_meta.m4,v 1.5 2006/09/27 00:36:28 achu Exp $
###*****************************************************************************
##  AUTHOR:
##    Jim Garlick <garlick.jim@gmail.com>
##
##  SYNOPSIS:
##    AC_LINUX_GPIB
##
##  DESCRIPTION:
##    Indicate that the linux-gpib library should be used, optionally at a
#     non-standard installation prefix.
#
##  USAGE:
##    --with-linux-gpib
##    --with-linux-gpib=DIR
##    --with-linux-gpib=yes
##    --without-linux-gpib
##    --with-linux-gpib=no
###*****************************************************************************
#
AC_DEFUN([AC_LINUX_GPIB], [
  AH_TEMPLATE([HAVE_LINUX_GPIB], [Define if linux-gpib is available])
  AC_ARG_WITH(linux-gpib, [  --with-linux-gpib=DIR   prefix for linux-gpib library files and headers], [
    if test "$withval" = "no"; then
      ac_linux_gpib_path=
      $2
    elif test "$withval" = "yes"; then
      ac_linux_gpib_path=/usr
    elif test -n "$withval"; then
      ac_linux_gpib_path="$withval"
    else 
      ac_linux_gpib_path=
    fi
  ])
  if test "$ac_linux_gpib_path" != ""; then
    saved_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$CPPFLAGS -I$ac_linux_gpib_path/include"
    AC_CHECK_HEADER([gpib/ib.h], [
      saved_LDFLAGS="$LDFLAGS"
      LDFLAGS="$LDFLAGS -L$ac_linux_gpib_path/lib"
      AC_CHECK_LIB(gpib, ibwrt, [
        AC_SUBST(GPIB_CPPFLAGS, [-I$ac_linux_gpib_path/include])
        AC_SUBST(GPIB_LDFLAGS, [-L$ac_linux_gpib_path/lib])
        AC_SUBST(GPIB_LIBS, [-lgpib])
        AC_DEFINE([HAVE_LINUX_GPIB])
        $1
      ], [
        echo ERROR: Could not find usable linux-gpib library.
        exit 1 
        $2
      ])
      LDFLAGS="$saved_LDFLAGS"
    ], [
      echo ERROR: Could not find usable linux-gpib include file
      exit 1
      $2
    ])
    CPPFLAGS="$saved_CPPFLAGS"
  fi
])
