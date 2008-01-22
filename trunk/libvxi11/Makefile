CFLAGS=-g -Wall -Wno-unused
LDADD=-lrpcsvc -lm
OBJS=vxi11.o vxi11_xdr.o vxi11_clnt.o

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

clean:
	rm -f *.o $(PROGS)
	rm -f vxi11_private.h vxi11_xdr.c vxi11_clnt.c vxi11_svc.c
