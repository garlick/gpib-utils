AM_CFLAGS =  @GCCWARNRPC@

noinst_LTLIBRARIES = libics.la

libics_la_SOURCES = \
	ics.c \
	ics.h \
	ics8000_clnt.c \
	ics8000_xdr.c \
	ics8000.h

CLEANFILES = ics8000.h ics8000_xdr.c ics8000_clnt.c ics8000_svc.c

ics.c: ics8000.h
ics8000.h: ics8000.x
	rm -f $@; rpcgen -o $@ -h ics8000.x
ics8000_xdr.c: ics8000.x ics8000.h
	rm -f $@; rpcgen -o $@ -c ics8000.x
ics8000_clnt.c: ics8000.x ics8000.h
	rm -f $@; rpcgen -o $@ -l ics8000.x
ics8000_svc.c:  ics8000.x ics8000.h
	rm -f $@; rpcgen -o $@ -m ics8000.x

EXTRA_DIST = ics8000.x
