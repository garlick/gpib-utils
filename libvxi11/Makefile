CFLAGS=-g -Wall -Wno-unused
LDADD=-lrpcsvc -lm -fPIC
LIBOBJS=  vxi11_device.o vxi11_core.o rpccache.o vxi11_xdr.o vxi11_clnt.o
LIBOBJS+= vxi11intr_xdr.o vxi11intr_svc.o
LIB=libvxi11.a

$(LIB): $(LIBOBJS)
	  ar rv $@ $(LIBOBJS)

vxi11_core.o vxi11_device.o: vxi11.h

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

clean:
	rm -f *.o $(PROGS)
	rm -f vxi11.h vxi11_xdr.c vxi11_clnt.c vxi11_svc.c
	rm -f vxi11intr.h vxi11intr_xdr.c vxi11intr_clnt.c vxi11intr_svc.c
