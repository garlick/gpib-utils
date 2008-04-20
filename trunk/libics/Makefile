CFLAGS=-g -Wall -Wno-unused
LDADD=-lrpcsvc -lm -fPIC
LIBOBJS=  ics.o ics8000_clnt.o ics8000_xdr.o
LIB=libics.a

$(LIB): $(LIBOBJS)
	  ar rv $@ $(LIBOBJS)

$(LIBOBJS): ics8000.h

ics8000.h: ics8000.x
	rm -f $@; rpcgen -o $@ -h ics8000.x
ics8000_xdr.c: ics8000.x ics8000.h
	rm -f $@; rpcgen -o $@ -c ics8000.x
ics8000_clnt.c: ics8000.x ics8000.h
	rm -f $@; rpcgen -o $@ -l ics8000.x
ics8000_svc.c:  ics8000.x ics8000.h
	rm -f $@; rpcgen -o $@ -m ics8000.x

clean:
	rm -f *.o *.a
	rm -f ics8000.h ics8000_xdr.c ics8000_clnt.c ics8000_svc.c
