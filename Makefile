BINDIR=/usr/local/bin
MANDIR=/usr/local/man/man1

all: all-nogpib all-other

# VXI-11 and linux-gpib
all-gpib:
	make clean
	make -C libics
	make -C src all CFLAGS_GPIB=-DHAVE_GPIB=1 LDADD_GPIB=-lgpib
# VXI-11 only
all-nogpib:
	make clean
	make -C libics
	make -C src all

all-other:
	make -C examples
	make -C htdocs

install:
	mkdir -p $(BINDIR) $(MANDIR)
	make -C src BINDIR=$(BINDIR) install
	make -C man MANDIR=$(MANDIR) install

clean:
	make -C libics clean
	make -C src clean
	make -C examples clean
	make -C htdocs clean

clobber: clean
	rm -f *.rpm *.bz2
	

