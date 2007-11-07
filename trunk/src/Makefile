CFLAGS=-Wall -Wno-unused -g -DHAVE_GPIB=0
#LDADD=-lgpib -lm
LDADD=-lrpcsvc -lm

PROGS=hp1630 hptopbm hptodino r5005 hp3455 hp3457 hp8656 dg535 \
      hp3488 sr620 hp603x gpsh
LIBOBJS=util.o gpib.o units.o errstr.o vxi11_xdr.o vxi11_clnt.o
TESTPROGS=testfreq testampl

all: $(PROGS) $(TESTPROGS)

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

gpib.o: vxi11.h

# VXI-11 support
vxi11: vxi11.h vxi11_xdr.c vxi11_clnt.c
vxi11.h: vxi11.x
	rpcgen -o $@ -h vxi11.x
vxi11_xdr.c: vxi11.x vxi11.h
	rpcgen -o $@ -c vxi11.x
vxi11_clnt.c: vxi11.x vxi11.h
	rpcgen -o $@ -l vxi11.x
vxi11_svc.c:  vxi11.x vxi11.h
	rpcgen -o $@ -m vxi11.x

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

