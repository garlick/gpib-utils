AC_INIT([gpib-utils], [1.3])

# put some configure products into ./config to avoid polluting topdir
AC_CONFIG_AUX_DIR([config])

# work with automake
AM_INIT_AUTOMAKE

# avoids spurious rebuilds of configure in some cases
AM_MAINTAINER_MODE

# put preprocessor config results in config.h
AC_CONFIG_HEADERS([config.h])

# automake wants this
AC_PROG_LIBTOOL
AC_ENABLE_SHARED([yes])

# look for pow(), in -lm if needed
AC_SEARCH_LIBS([pow], [m])

# look for readline() in -lreadline, then set HAVE_LIBREADLINE in config.h
# and substitute LIBREADLINE in Makefiles
AC_CHECK_LIB( [readline], [readline], 
  [AC_DEFINE([HAVE_LIBREADLINE], [1], [Define if you have libreadline])
   AC_SUBST([LIBREADLINE], ["-lreadline -lcurses"])], 
  [], [-lcurses])

# look for ibclr() in -lgpib , then set CONFIG_LIBGPIB in config.h
AC_CHECK_LIB([gpib], [ibclr])

# process automake's Makefile.in's to Makefile's
AC_OUTPUT([\
	Makefile \
	libvxi11/Makefile \
	libics/Makefile \
	src/Makefile
	man/Makefile
])
