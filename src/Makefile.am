AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = \
	-I$(top_srcdir)
	@GPIB_CPPFLAGS@

LDADD = $(top_builddir)/libvxi11/libvxi11.la \
	$(top_builddir)/libics/libics.la \
	$(top_builddir)/liblsd/liblsd.la \
	$(top_builddir)/libutil/libutil.la \
	@GPIB_LDFLAGS@ @GPIB_LIBS@

common_sources = \
	gpib.c \
	gpib.h

bin_PROGRAMS = \
	hp1630 \
	hptopbm \
	hptodino \
	hp3488 \
	ics8064 \
        icsconfig \
	a34401 \
	ibquery

hp1630_SOURCES = hp1630.c $(common_sources)
hptopbm_SOURCES =  hptopbm.c $(common_sources)
hptodino_SOURCES =  hptodino.c $(common_sources)
hp3488_SOURCES = hp3488.c $(common_sources)
ics8064_SOURCES = ics8064.c $(common_sources)
icsconfig_SOURCES = icsconfig.c  $(common_sources)
a34401_SOURCES = a34401.c $(common_sources)
ibquery_SOURCES = ibquery.c  $(common_sources)


