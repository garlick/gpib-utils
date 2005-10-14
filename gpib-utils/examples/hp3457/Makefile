# plot sample.dat to postscript file.
# Samples were taken 0.1s apart with floating dc input.

all: sample.ps

sample.ps: sample.dat

%.ps : %.gp
	gnuplot $<

clean:
	rm -f sample.ps
