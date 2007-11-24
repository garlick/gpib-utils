#set terminal postscript color
#set output 'filter.ps'
set terminal pbm mono
set output 'filter.ppm'

set xlabel "frequency (khz)"
set ylabel "ac volts"
set xrange[100:]
set yrange[0:1.2]
set title "Output of High Pass RC Filter with 1.0V RMS Input"

#plot "filter1.dat" title "R=33.4 ohms, C=0.01 uF" with linesp,\
#     "filter2.dat" title "R=33.4 ohms, C=0.02 uF" with linesp,\
#     "filter3.dat" title "R=66.8 ohms, C=0.01 uF" with linesp,\
#     "filter4.dat" title "R=66.8 ohms, C=0.02 uF" with linesp;

plot "filter1.dat" title "R=33.4 ohms, C=0.01 uF" with linesp,\
     "filter2.dat" title "R=33.4 ohms, C=0.02 uF" with linesp,\
     "filter4.dat" title "R=66.8 ohms, C=0.02 uF" with linesp;

#plot "filter4.dat" title "R=66.8 ohms, C=0.02 uF" with linesp;
