CFLAGS=-g -Wall -Wno-unused
LDADD=-lrpcsvc -lm
OBJS=vxi11.o vxi11_xdr.o vxi11_clnt.o vxi11intr_xdr.o vxi11intr_svc.o

all: $(OBJS)

# VXI-11 core/async
vxi11.o: vxi11_private.h
vxi11_private.h: vxi11.x
	rm -f $@; rpcgen -o $@ -h vxi11.x
vxi11_xdr.c: vxi11.x vxi11_private.h
	rm -f $@; rpcgen -o $@ -c vxi11.x
	sed --in-place -e 's/"vxi11.h"/"vxi11_private.h"/' $@
vxi11_clnt.c: vxi11.x vxi11_private.h
	rm -f $@; rpcgen -o $@ -l vxi11.x
	sed --in-place -e 's/"vxi11.h"/"vxi11_private.h"/' $@
vxi11_svc.c:  vxi11.x vxi11_private.h
	rm -f $@; rpcgen -o $@ -m vxi11.x
	sed --in-place -e 's/"vxi11.h"/"vxi11_private.h"/' $@
# VXI-11 intr
vxi11.o: vxi11intr.h
vxi11intr.h: vxi11intr.x
	rm -f $@; rpcgen -o $@ -h vxi11intr.x
vxi11intr_xdr.c: vxi11intr.x vxi11intr.h
	rm -f $@; rpcgen -o $@ -c vxi11intr.x
vxi11intr_svc.c:  vxi11intr.x vxi11intr.h
	rm -f $@; rpcgen -o $@ -m vxi11intr.x

clean:
	rm -f *.o $(PROGS)
	rm -f vxi11_private.h vxi11_xdr.c vxi11_clnt.c vxi11_svc.c
	rm -f vxi11intr.h vxi11intr_xdr.c vxi11intr_clnt.c vxi11intr_svc.c
