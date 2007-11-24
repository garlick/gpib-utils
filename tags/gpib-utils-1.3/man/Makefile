MANPAGES=a6032.1 hp1630.1 hp3457.1 hptopbm.1 hp8656.1 hptodino.1 hp3488.1 \
	 dg535.1 sr620.1 ics8065.1
 
default:

install:
	@test $(MANDIR)
	@for manpage in $(MANPAGES); do \
	  install -m 0444 $$manpage $(MANDIR); \
	done
