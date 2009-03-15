##
# Prologue
##
AC_INIT([gpib-utils])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_SRCDIR([NEWS])
AC_CANONICAL_SYSTEM
X_AC_META
X_AC_EXPAND_INSTALL_DIRS

##
# Automake support
##
AM_INIT_AUTOMAKE($META_NAME, $META_VERSION)
AM_CONFIG_HEADER([config/config.h])
AM_MAINTAINER_MODE

##
# Checks for programs
##
AC_PROG_CC
if test "$GCC" = yes; then
  GCCWARN="-Wall -Werror"
  GCCWARNRPC="-Wall -Wno-unused -Werror"
  AC_SUBST([GCCWARN])
  AC_SUBST([GCCWARNRPC])
fi
AC_PROG_RANLIB

##
# Checks for header files.
##
AC_HEADER_STDC
AC_CHECK_HEADERS( \
  getopt.h \
)

##
# Checks for typedefs, structures, and compiler characteristics
## 
AC_C_BIGENDIAN
AC_C_CONST

##
# Checks for library functions
##
AC_LINUX_GPIB
AC_CHECK_FUNCS( \
  getopt_long \
  vsnprintf \
  vsscanf \
)
AC_SEARCH_LIBS([pow], [m])
AC_SEARCH_LIBS([clnt_create],[nsl])
AC_SEARCH_LIBS([inet_aton],[resolv])

##
# For list.c, hostlist.c
##
AC_DEFINE(WITH_LSD_FATAL_ERROR_FUNC, 1, [Define lsd_fatal_error])
AC_DEFINE(WITH_LSD_NOMEM_ERROR_FUNC, 1, [Define lsd_fatal_error])
AC_DEFINE(WANT_RECKLESS_HOSTRANGE_EXPANSION, 1, 
					[Allow reckless hostname expansion])

##
# Epilogue
##
AC_CONFIG_FILES( \
  Makefile \
  libvxi11/Makefile \
  libics/Makefile \
  liblsd/Makefile \
  src/Makefile \
  man/Makefile \
  man/a34401.1 \
  man/a6032.1 \
  man/dg535.1 \
  man/hp1630.1 \
  man/hp3457.1 \
  man/hp3488.1 \
  man/hp6632.1 \
  man/hp6626.1 \
  man/hp6038.1 \
  man/hp8656.1 \
  man/hptodino.1 \
  man/hptopbm.1 \
  man/ics8064.1 \
  man/icsconfig.1 \
  man/sr620.1 \
  man/ibquery.1 \
  man/gpib-utils.conf.5 \
)
AC_OUTPUT
