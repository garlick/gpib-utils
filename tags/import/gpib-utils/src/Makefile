CFLAGS=-Wall -g
LDADD=-lgpib

PROGS=hp1630 hptopbm hptodino r5005 hp3455 hp3457

all: $(PROGS)

hptodino: hptodino.o freq.o
	$(CC) -o $@ hptodino.o freq.o

hp1630: hp1630.o
	$(CC) -o $@ hp1630.o $(LDADD)

r5005: r5005.o
	$(CC) -o $@ r5005.o $(LDADD)

hp3455: hp3455.o
	$(CC) -o $@ hp3455.o $(LDADD)

hp3457: hp3457.o freq.o
	$(CC) -o $@ hp3457.o freq.o $(LDADD)

install:
	@test $(BINDIR)
	@for prog in $(PROGS); do \
	   install -m 0555 $$prog $(BINDIR); \
	done

clean:
	rm -f *.o $(PROGS)
