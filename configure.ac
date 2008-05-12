AC_INIT([gpib-utils], [1.3])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])
AC_PROG_LIBTOOL
AC_ENABLE_SHARED([yes])

AC_SEARCH_LIBS([pow], [m])

AC_CHECK_LIB([readline], [readline], , ,[-lcurses])
AC_CHECK_LIB([gpib], [ibclr])

AC_OUTPUT([\
	Makefile \
	libvxi11/Makefile \
	libics/Makefile \
	src/Makefile
	man/Makefile
])
