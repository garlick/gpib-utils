# plot sample.dat to postscript file.
# Samples were taken 0.1s apart with floating dc input.

all: filter.ps

filter.ps: filter1.dat filter2.dat filter3.dat

%.ps : %.gp
	gnuplot $<

# if 'pbm color' is uncommented instead of ps!
%.ppm : %.gp
	gnuplot $<

%.gif : %.ppm
	ppmtogif < $< >$@

clean:
	rm -f filter.ps filter.ppm filter.gif
