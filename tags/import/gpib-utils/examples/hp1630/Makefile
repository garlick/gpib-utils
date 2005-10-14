HPTODINO=../../src/hptodino
HPTOPBM=../../src/hptopbm
FILES = state.tra timing.tra screen.ps screen.gif

all: $(FILES)

sview: state.tra state.dino
	dinotrace $<

tview: timing.tra timing.dino
	dinotrace $<

# 10 ns clock
timing.tra: timing.dump
	$(HPTODINO) -c 10ns <$< >$@

# 1 us clock, triggered on rising and falling edges
state.tra: state.dump
	$(HPTODINO) -c 500ns <$< >$@

# Note: pbmtolps is part of the 'netpbm-progs' package on my FC3 linux box.
# dump taken with "hp1630 --print-screen >screen.hp"
%.ps : %.hp
	cat $< | $(HPTOPBM) | pbmtolps -dpi 100 > $@

%.gif : %.hp
	cat $< | $(HPTOPBM) | pgmtoppm white | ppmtogif  > $@

clean:
	rm -f $(FILES)
