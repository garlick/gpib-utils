HPTODINO=../../src/hptodino
HPTOPBM=../../src/hptopbm
FILES = state.tra timing.tra screen.pbm

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

%.pbm : %.hp
	$(HPTOPBM) <$< >$@

clean:
	rm -f $(FILES)
