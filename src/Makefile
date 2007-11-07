CFLAGS=-Wall -g
LDADD=-lgpib -lm

PROGS=hp1630 hptopbm hptodino r5005 hp3455 hp3457 hp8656 dg535 tek2230  \
      hp3488 sr620
TESTPROGS=testfreq testampl

all: $(PROGS) $(TESTPROGS)

hptodino: hptodino.o units.o hpcrc.o
	$(CC) -o $@ hptodino.o units.o hpcrc.o -lm

hp1630: hp1630.o errstr.o gpib.o util.o hpcrc.o
	$(CC) -o $@ hp1630.o errstr.o gpib.o util.o hpcrc.o $(LDADD)

r5005: r5005.o units.o gpib.o errstr.o
	$(CC) -o $@ r5005.o units.o gpib.o errstr.o $(LDADD)

hp3455: hp3455.o units.o gpib.o
	$(CC) -o $@ hp3455.o units.o gpib.o $(LDADD)

hp3457: hp3457.o units.o gpib.o
	$(CC) -o $@ hp3457.o units.o gpib.o $(LDADD)

hp8656: hp8656.o units.o gpib.o
	$(CC) -o $@ hp8656.o units.o gpib.o $(LDADD)

dg535: dg535.o units.o gpib.o
	$(CC) -o $@ dg535.o units.o gpib.o $(LDADD)

sr620: sr620.o units.o gpib.o util.o
	$(CC) -o $@ sr620.o units.o gpib.o util.o $(LDADD)

tek2230: tek2230.o gpib.o
	$(CC) -o $@ tek2230.o gpib.o $(LDADD)

hp3488: hp3488.o gpib.o
	$(CC) -o $@ hp3488.o gpib.o $(LDADD)

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
