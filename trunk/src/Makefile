CFLAGS_LSD=-DWITH_LSD_NOMEM_ERROR_FUNC -DWITH_LSD_FATAL_ERROR_FUNC
CFLAGS_LSD+=-DWANT_RECKLESS_HOSTRANGE_EXPANSION
CFLAGS=-Wall -Wno-unused -g $(CFLAGS_GPIB) $(CFLAGS_LSD)
LDADD=-lrpcsvc -lm -lpthread -lreadline $(LDADD_GPIB)
CFLAGS+=-I../libics -I../libvxi11
LDADD+=-L../libics -lics -L../libvxi11 -lvxi11

PROGS=hp6632 hp1630 hptopbm hptodino hp3457 hp3488 hp8656 \
      dg535 sr620 ics8065 ics8064 a6032 a34401
LIBOBJS=util.o gpib.o units.o argv.o list.o hostlist.o
TESTPROGS=testfreq testampl

all: $(PROGS)

# utilities
hptopbm: hptopbm.o $(LIBOBJS)
	$(CC) -o $@ hptopbm.o $(LIBOBJS) $(LDADD)
hptodino: hptodino.o hpcrc.o $(LIBOBJS)
	$(CC) -o $@ hptodino.o hpcrc.o $(LIBOBJS) $(LDADD)
hp1630: hp1630.o hpcrc.o $(LIBOBJS)
	$(CC) -o $@ hp1630.o hpcrc.o $(LIBOBJS) $(LDADD)
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
a6032: a6032.o $(LIBOBJS)
	$(CC) -o $@ a6032.o $(LIBOBJS) $(LDADD)
a34401: a34401.o $(LIBOBJS)
	$(CC) -o $@ a34401.o $(LIBOBJS) $(LDADD)
ics8065: ics8065.o $(LIBOBJS)
	$(CC) -o $@ ics8065.o $(LIBOBJS) $(LDADD)
ics8064: ics8064.o $(LIBOBJS)
	$(CC) -o $@ ics8064.o $(LIBOBJS) $(LDADD)
hp6632: hp6632.o $(LIBOBJS)
	$(CC) -o $@ hp6632.o $(LIBOBJS) $(LDADD)

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
	rm -f *.o $(PROGS) $(TESTPROGS)
