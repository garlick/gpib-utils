MANPAGES=hp1630.1 hp3455.1 hp3457.1 hptopbm.1 r5005.1 hp8656.1 hptodino.1

default:

install:
	@test $(MANDIR)
	@for manpage in $(MANPAGES); do \
	  install -m 0444 $$manpage $(MANDIR); \
	done
