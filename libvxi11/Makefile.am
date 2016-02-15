AM_CFLAGS = @GCCWARNRPC@

lib_LTLIBRARIES = libvxi11.la

noinst_LTLIBRARIES = libvxi11intr.la

libvxi11_la_SOURCES = \
	vxi11_device.c \
	vxi11_core.c \
	rpccache.c \
	vxi11_xdr.c \
	vxi11_clnt.c \
	vxi11.h \
	rpccache.h  \
	vxi11_core.h  \
	vxi11_device.h  \
	vxi11.h

libvxi11intr_la_SOURCES = \
	vxi11intr_xdr.c \
	vxi11intr_svc.c \
	vxi11intr.h

CLEANFILES = \
	vxi11.h \
	vxi11_xdr.c \
	vxi11_clnt.c \
	vxi11_svc.c \
	vxi11intr.h \
	vxi11intr_xdr.c \
	vxi11intr_clnt.c \
	vxi11intr_svc.c

vxi11_core.c vxi11_device.c: vxi11.h
# VXI-11 core/async
vxi11.h: vxi11.x
	rm -f $@; rpcgen -o $@ -h vxi11.x
vxi11_xdr.c: vxi11.x vxi11.h
	rm -f $@; rpcgen -o $@ -c vxi11.x
vxi11_clnt.c: vxi11.x vxi11.h
	rm -f $@; rpcgen -o $@ -l vxi11.x
vxi11_svc.c:  vxi11.x vxi11.h
	rm -f $@; rpcgen -o $@ -m vxi11.x
# VXI-11 intr
vxi11intr.h: vxi11intr.x
	rm -f $@; rpcgen -o $@ -h vxi11intr.x
vxi11intr_xdr.c: vxi11intr.x vxi11intr.h
	rm -f $@; rpcgen -o $@ -c vxi11intr.x
vxi11intr_svc.c:  vxi11intr.x vxi11intr.h
	rm -f $@; rpcgen -o $@ -m vxi11intr.x

if WITH_PKG_CONFIG
pkgconfig_DATA = libvxi11.pc
endif
include_HEADERS = vxi11_device.h vxi11_core.h vxi11.h

EXTRA_DIST = vxi11.x vxi11intr.x
