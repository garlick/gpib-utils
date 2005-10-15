BINDIR=/usr/local/bin
MANDIR=/usr/local/man/man1

all:
	make -C src all
	make -C examples all

install:
	mkdir -p $(BINDIR) $(MANDIR)
	make -C src BINDIR=$(BINDIR) install
	make -C man MANDIR=$(MANDIR) install

clean:
	make -C src clean
	make -C examples clean
