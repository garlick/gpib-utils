CFLAGS=-Wall -Wno-unused -g $(CFLAGS_GPIB)
LDADD=-lrpcsvc -lm $(LDADD_GPIB)

PROGS=hp1630 hptopbm hptodino r5005 hp3455 hp3457 hp8656 dg535 \
      hp3488 sr620 hp603x gpsh ics8065 a6032
LIBOBJS=util.o gpib.o units.o vxi11_xdr.o vxi11_clnt.o vxi11srq_svc.o \
	ics.o ics8000_clnt.o ics8000_xdr.o 
TESTPROGS=testfreq testampl

all: $(PROGS)

# utilities
hptopbm: hptopbm.o $(LIBOBJS)
	$(CC) -o $@ hptopbm.o $(LIBOBJS) $(LDADD)
hptodino: hptodino.o hpcrc.o $(LIBOBJS)
	$(CC) -o $@ hptodino.o hpcrc.o $(LIBOBJS) $(LDADD)
hp1630: hp1630.o hpcrc.o $(LIBOBJS)
	$(CC) -o $@ hp1630.o hpcrc.o $(LIBOBJS) $(LDADD)
r5005: r5005.o $(LIBOBJS)
	$(CC) -o $@ r5005.o $(LIBOBJS) $(LDADD)
hp3455: hp3455.o $(LIBOBJS)
	$(CC) -o $@ hp3455.o $(LIBOBJS) $(LDADD)
hp3457: hp3457.o $(LIBOBJS)
	$(CC) -o $@ hp3457.o $(LIBOBJS) $(LDADD)
hp8656: hp8656.o $(LIBOBJS)
	$(CC) -o $@ hp8656.o $(LIBOBJS) $(LDADD)
dg535: dg535.o $(LIBOBJS)
	$(CC) -o $@ dg535.o $(LIBOBJS) $(LDADD)
sr620: sr620.o $(LIBOBJS)
	$(CC) -o $@ sr620.o $(LIBOBJS) $(LDADD)
hp3488: hp3488.o $(LIBOBJS)
	$(CC) -o $@ hp3488.o $(LIBOBJS) $(LDADD)
hp603x: hp603x.o $(LIBOBJS)
	$(CC) -o $@ hp603x.o $(LIBOBJS) $(LDADD)
gpsh: gpsh.o $(LIBOBJS)
	$(CC) -o $@ gpsh.o $(LIBOBJS) $(LDADD) -lreadline -ltermcap
ics8065: ics8065.o $(LIBOBJS)
	$(CC) -o $@ ics8065.o $(LIBOBJS) $(LDADD)
ics.o: ics8000.h
a6032: a6032.o $(LIBOBJS)
	$(CC) -o $@ a6032.o $(LIBOBJS) $(LDADD)

# VXI-11 core/async
gpib.o: vxi11.h
vxi11.h: vxi11.x
	rm -f $@; rpcgen -o $@ -h vxi11.x
vxi11_xdr.c: vxi11.x vxi11.h
	rm -f $@; rpcgen -o $@ -c vxi11.x
vxi11_clnt.c: vxi11.x vxi11.h
	rm -f $@; rpcgen -o $@ -l vxi11.x
vxi11_svc.c:  vxi11.x vxi11.h
	rm -f $@; rpcgen -o $@ -m vxi11.x
# VXI-11 srq
srq: vxi11srq.h vxi11srq_xdr.c vxi11srq_clnt.c vxi11srq_svc.c
vxi11srq.h: vxi11srq.x
	rm -f $@; rpcgen -o $@ -h vxi11srq.x
vxi11srq_xdr.c: vxi11srq.x vxi11srq.h
	rm -f $@; rpcgen -o $@ -c vxi11srq.x
vxi11srq_clnt.c: vxi11srq.x vxi11srq.h
	rm -f $@; rpcgen -o $@ -l vxi11srq.x
vxi11srq_svc.c:  vxi11srq.x vxi11srq.h
	rm -f $@; rpcgen -o $@ -m vxi11srq.x
# VXI-11 extension for ICS 8000 series
ics8000.h: ics8000.x
	rm -f $@; rpcgen -o $@ -h ics8000.x
ics8000_xdr.c: ics8000.x ics8000.h
	rm -f $@; rpcgen -o $@ -c ics8000.x
ics8000_clnt.c: ics8000.x ics8000.h
	rm -f $@; rpcgen -o $@ -l ics8000.x
ics8000_svc.c:  ics8000.x ics8000.h
	rm -f $@; rpcgen -o $@ -m ics8000.x

# test programs
testampl: units.c
	$(CC) -o $@ -DAMPLMAIN $< -lm
testfreq: units.c
	$(CC) -o $@ -DFREQMAIN $< -lm

install:
	@test $(BINDIR)
	@for prog in $(PROGS); do \
	   install -m 0555 $$prog $(BINDIR); \
	done

clean:
	rm -f *.o $(PROGS)
	rm -f $(TESTPROGS)
	rm -f vxi11.h vxi11_xdr.c vxi11_clnt.c vxi11_svc.c
	rm -f vxi11srq.h vxi11srq_xdr.c vxi11srq_clnt.c vxi11srq_svc.c
	rm -f ics8000.h ics8000_xdr.c ics8000_clnt.c ics8000_svc.c

