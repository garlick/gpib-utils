# plot sample.dat to postscript file.
# Samples were taken 0.1s apart with floating dc input.

all: sample.ps

sample.ps: sample.dat

%.ps : %.gp
	gnuplot $<

# if 'pbm color' is uncommented instead of ps!
%.ppm : %.gp
	gnuplot $<

%.gif : %.ppm
	ppmtogif < $< >$@

clean:
	rm -f sample.ps sample.ppm sample.gif
