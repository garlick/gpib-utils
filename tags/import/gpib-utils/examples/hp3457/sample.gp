set terminal postscript color
set output 'sample.ps'
set xlabel "time (s)"
set ylabel "dc volts (v)"
set xrange[0:]
set yrange[0:1]
set title "HP 3457A Voltmeter Floating Input"

plot "sample.dat" title "Sample Plot" with linesp;
