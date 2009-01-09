AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = \
	-I$(top_srcdir)/libvxi11 -I$(top_srcdir)/libics -I$(top_srcdir)/liblsd \
	@GPIB_CPPFLAGS@

LDADD = $(top_builddir)/libvxi11/libvxi11.a \
	$(top_builddir)/libics/libics.a \
	$(top_builddir)/liblsd/liblsd.a \
	@GPIB_LDFLAGS@ @GPIB_LIBS@

common_sources = \
	util.c \
	gpib.c \
	hpcrc.c \
	util.h \
	gpib.h \
	hpcrc.h


bin_PROGRAMS = \
	hp6632 \
	hp1630 \
	hptopbm \
	hptodino \
	hp3457 \
	hp3488 \
	hp8656 \
	dg535 \
	sr620 \
	ics8065 \
	ics8064 \
	a6032 \
	a34401

hp1630_SOURCES = hp1630.c hp1630.h $(common_sources)
hptopbm_SOURCES =  hptopbm.c $(common_sources)
hptodino_SOURCES =  hptodino.c $(common_sources)
hp3457_SOURCES = hp3457.c hp3457.h $(common_sources)
hp3488_SOURCES = hp3488.c hp3488.h $(common_sources)
hp8656_SOURCES = hp8656.c hp8656.h $(common_sources)
dg535_SOURCES = dg535.c dg535.h $(common_sources)
sr620_SOURCES = sr620.c sr620.h $(common_sources)
ics8065_SOURCES = ics8065.c $(common_sources)
ics8064_SOURCES = ics8064.c $(common_sources)
a6032_SOURCES = a6032.c $(common_sources)
a34401_SOURCES = a34401.c $(common_sources)
hp6632_SOURCES = hp6632.c  $(common_sources)


