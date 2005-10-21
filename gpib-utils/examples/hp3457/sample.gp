set terminal postscript color
set output 'sample.ps'
set xlabel "time (s)"
set ylabel "dc volts (v)"
set xrange[0:]
set yrange[0:4]
set title "Weak 9V battery dicharging through a 1 ohm resistor"

plot "sample.dat" title "Sample Plot" with linesp;
