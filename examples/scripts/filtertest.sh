#!/bin/bash

PATH=../../src:$PATH

# Characterize freq response of RC high pass filter

# Initialize the instruments
hp3457 --clear --function acv --trigger syn
hp8656 --clear --frequency 100khz --incrfreq 10khz --amplitude 1v

# Take measurements from 100khz to 1mhz in 10khz increments
for freq in `seq 100 10 1000`; do
	volts=`hp3457 --samples 1`
	echo "$freq	$volts"
	hp8656 --frequency up
done

# Put instruments back in front-panel mode
hp3457 --local
hp8656 --local
