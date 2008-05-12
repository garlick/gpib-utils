AC_INIT([gpib-utils], [1.3])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_CONFIG_HEADERS([config.h])
AC_PROG_LIBTOOL
AC_ENABLE_SHARED([yes])

AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([readline], [readline])
AC_CHECK_LIB([gpib], [ibclr])

AC_OUTPUT([\
	Makefile \
	libvxi11/Makefile \
	libics/Makefile \
	src/Makefile
	man/Makefile
])
