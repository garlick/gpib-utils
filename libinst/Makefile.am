AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = \
	-I$(top_srcdir)
	@GPIB_CPPFLAGS@

noinst_LTLIBRARIES = libinst.la

libinst_la_SOURCES = \
	gpib.c \
	gpib.h \
	ib_488_2.c \
	ib_488_2.h
