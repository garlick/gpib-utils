/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2005 Jim Garlick <garlick@speakeasy.net>

   gpib-utils is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   gpib-utils is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with gpib-utils; if not, write to the Free Software Foundation, 
   Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

/* GPIB control codes for the Stanford Research Systems SR620
 * Universal Time Interval Counter
 */


/* i = input channel (0=ext, 1=A, 2=B)
 * j,k,l,m,n = integer
 * x = real
 * All variables may be in integer, floating pt., or exponential formats,
 * e.g. five can be 5, 5.0, or .5E1
 */

/*
 * Interface control commands
 */
#define SR620_RESET             "*RST"

#define SR620_ID_QUERY          "*IDN?" 
#define SR620_ID_RESPONSE       "StanfordResearchSystems,SR620" /* +sn,fwrev */

#define SR620_SETUP_QUERY       "STUP?"

/*
 * Trigger Control Commands
 */
/* set/query trigger level */
#define SR620_TRIG_LEVEL        "LEVL"  /* LEVL i{,x} */
#define SR620_TRIG_LEVEL_QUERY  "LEVL?"

#define SR620_TRIG_MANUAL       "MTRG"  /* MTRG j (0=start,1=stop gate) */

/* set/query front panel ref. output level */
#define SR620_TRIG_REFLEV       "RLVL"  /* RLVL(?) {j} (0=ecl,1=ttl output) */
#define SR620_TRIG_REFLEV_QUERY "RLVL?"

/* set/query ac/dc coupling of chan A and B inputs */
#define SR620_TRIG_COUP         "TCPL"  /* TCPL(?) i{,j} (0=dc,1=ac) */
#define SR620_TRIG_COUP_QUERY   "TCPL?"

/* set/query input termination of EXT, A, and B inputs */
#define SR620_TRIG_TERM         "TERM"  /* TERM(?) i{,j} (0=50ohm,1=1Mohm) */
#define SR620_TRIG_TERM_QUERY   "TERM?" /* (in f/T mode, 2=UHF prescalers)*/

/* set/query trigger mode of A and B inputs */
#define SR620_TRIG_MODE         "TMOD"  /* TMOD(?) i{,j} (0=norm,1=autolev) */
#define SR620_TRIG_MODE_QUERY   "TMOD?"

/* set/query trigger slopes of EXT, A, and B inputs */
#define SR620_TRIG_SLOPE        "TSLP"  /* TSLP(?) i{,j} (0=pos,1=neg) */
#define SR620_TRIG_SLOPE_QUERY  "TSLP?"

#define SR620_TRIG_START        "STRT"  /* START button (alias *TRG) */
#define SR620_TRIG_STOP         "STOP"  /* RESET button */

#define SR620_TRIG_ARMMODE      "ARMM"  /* ARMM(?) {j} (see below) */
#define SR620_TRIG_ARMMODE_QUERY "ARMM?"
/* j    arming mode                 measurement mode
 * 0    +/- time                    time
 * 1    +   time                    time, width, rise/fall time, phase
 * 2    1 period                    frequency, period
 * 3    0.01s gate                  frequency, period, count
 * 4    0.1s gate                   frequency, period, count
 * 5    1.0s gate                   frequency, period, count
 * 6    ext trig +/- time           time
 * 7    ext trig + time             time, width, rise/fall time, phase
 * 8    ext gate/+ time/hldf        time, width, frequency, period, count
 * 9    ext triggered 1 period      frequency, period
 * 10   ext triggered 0.01s gate    frequency, period, count
 * 11   ext triggered 0.1s gate     frequency, period, count
 * 12   ext triggered 1.0s gate     frequency, period, count
 */

/* auto meas. mode (XXX set to off if computer is taking data) */
#define SR620_TRIG_AUTOMEAS     "AUTM"  /* AUTM(?) {j} (1=on, 0=off) */
#define SR620_TRIG_AUTOMEAS_QUERY "AUTM?"

/* set/query display rel */
#define SR620_DREL              "DREL"/* DREL(?) j (0=clr,1=set,2=last,3=cur) */
#define SR620_DREL_QUERY        "DREL?"

/* change arming parity (equiv to toggling state of front panel COMPL LED) */
#define SR620_COMP              "COMP"

/* set/query width of freq, period, or count gate to x (1ms - 500s) */
#define SR620_GATE              "GATE"  /* GATE(?) x (negative val = external)*/
#define SR620_GATE_QUERY        "GATE?"

/* set/query the type of jitter calculation */
#define SR620_JITTER            "JTTR"  /* JTTR(?) j (0=stddev,1=allanvar) */
#define SR620_JITTER_QUERY      "JTTR?"

/* set/query measurement mode */
#define SR620_MODE              "MODE"  /* MODE(?) j (see below) */
#define SR620_MODE_QUERY        "MODE?"
/* 0=time,1=width,2=rise/fall time,3=freq,4=period,5=phase,6=count */

#define SR620_SIZE              "SIZE"  /* SIZE(?) x */
#define SR620_SIZE_QUERY        "SIZE?"

#define SR620_SRCE              "SRCE"  /* SRCE(?) j (0=A,1=B,2=ref,3=A/B) */

/*
 * Data transmission commands
 */

#define SR620_MEAS              "MEAS" /* MEAS(?) j (0=mean,1=jit,2=max,3=min)*/
#define SR620_MEAS_QUERY        "MEAS?"

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
