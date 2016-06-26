AM_CFLAGS = @GCCWARN@

AM_CPPFLAGS = \
	-I$(top_srcdir)
	@GPIB_CPPFLAGS@

LDADD = $(top_builddir)/libinst/libinst.la \
	$(top_builddir)/libvxi11/libvxi11.la \
	$(top_builddir)/libics/libics.la \
	$(top_builddir)/libini/libini.la \
	$(top_builddir)/liblsd/liblsd.la \
	$(top_builddir)/libutil/libutil.la \
	@GPIB_LDFLAGS@ @GPIB_LIBS@

bin_PROGRAMS = \
	hp3488 \
	ics8064 \
        icsconfig \
	ibquery
