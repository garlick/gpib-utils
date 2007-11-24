# plot sample.dat to ppm file.
# Samples were taken 0.1s apart with floating dc input.

all: filter.ppm

filter.ppm: filter1.dat filter2.dat filter3.dat

# if 'pbm color' is uncommented instead of ps!
%.ppm : %.gp
	gnuplot $<

clean:
	rm -f filter.ppm
