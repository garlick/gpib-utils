AM_CFLAGS = -Wall

noinst_LIBRARIES = libgpibutils.a
libgpibutils_a_SOURCES = util.c gpib.c list.c argv.c hostlist.c hpcrc.c \
	util.h gpib.h list.h argv.h hostlist.h hpcrc.h

AM_CPPFLAGS = -I$(top_srcdir)/libvxi11 -I$(top_srcdir)/libics
LDADD = libgpibutils.a $(top_builddir)/libvxi11/libvxi11.a \
	$(top_builddir)/libics/libics.a @LIBREADLINE@

bin_PROGRAMS = hp6632 hp1630 hptopbm hptodino hp3457 hp3488 hp8656 \
      dg535 sr620 ics8065 ics8064 a6032 a34401
hp1630_SOURCES = hp1630.c hp1630.h
hptopbm_SOURCES =  hptopbm.c
hptodino_SOURCES =  hptodino.c
hp3457_SOURCES = hp3457.c hp3457.h
hp3488_SOURCES = hp3488.c hp3488.h
hp8656_SOURCES = hp8656.c hp8656.h
dg535_SOURCES = dg535.c dg535.h
sr620_SOURCES = sr620.c sr620.h
ics8065_SOURCES = ics8065.c
ics8064_SOURCES = ics8064.c
a6032_SOURCES = a6032.c
a34401_SOURCES = a34401.c
hp6632_SOURCES = hp6632.c 


