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
	hp6632 \
	hp6626 \
	hp6038 \
	hp1630 \
	hptopbm \
	hptodino \
	hp3457 \
	hp3488 \
	hp8656 \
	dg535 \
	sr620 \
	ics8064 \
        icsconfig \
	a6032 \
	a34401 \
	ibquery

noinst_PROGRAMS = \
	hp1650

hp1630_SOURCES = hp1630.c $(common_sources)
hp1650_SOURCES = hp1650.c $(common_sources)
hptopbm_SOURCES =  hptopbm.c $(common_sources)
hptodino_SOURCES =  hptodino.c $(common_sources)
hp3457_SOURCES = hp3457.c $(common_sources)
hp3488_SOURCES = hp3488.c $(common_sources)
hp8656_SOURCES = hp8656.c $(common_sources)
dg535_SOURCES = dg535.c $(common_sources)
sr620_SOURCES = sr620.c $(common_sources)
ics8064_SOURCES = ics8064.c $(common_sources)
icsconfig_SOURCES = icsconfig.c  $(common_sources)
a6032_SOURCES = a6032.c $(common_sources)
a34401_SOURCES = a34401.c $(common_sources)
hp6632_SOURCES = hp6632.c  $(common_sources)
hp6626_SOURCES = hp6626.c  $(common_sources)
hp6038_SOURCES = hp6038.c  $(common_sources)
ibquery_SOURCES = ibquery.c  $(common_sources)


