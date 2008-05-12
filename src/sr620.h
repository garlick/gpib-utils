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

/**
 ** Trigger Control Commands
 **/

/* set/query trigger level */
#define SR620_LEVL              "LEVL"  /* LEVL i{,x} */
#define SR620_LEVL_QUERY        "LEVL?"

#define SR620_MTRG              "MTRG"  /* MTRG j (0=start,1=stop gate) */

/* set/query front panel ref. output level */
#define SR620_RLVL              "RLVL"  /* RLVL(?) {j} (0=ecl,1=ttl output) */
#define SR620_RLVL_QUERY        "RLVL?"

/* set/query ac/dc coupling of chan A and B inputs */
#define SR620_TCPL              "TCPL"  /* TCPL(?) i{,j} (0=dc,1=ac) */
#define SR620_TCPL_QUERY        "TCPL?"

/* set/query input termination of EXT, A, and B inputs */
#define SR620_TERM              "TERM"  /* TERM(?) i{,j} (0=50ohm,1=1Mohm) */
#define SR620_TERM_QUERY        "TERM?" /* (in f/T mode, 2=UHF prescalers)*/

/* set/query trigger mode of A and B inputs */
#define SR620_TMOD              "TMOD"  /* TMOD(?) i{,j} (0=norm,1=autolev) */
#define SR620_TRIG_MODE_QUERY   "TMOD?"

/* set/query trigger slopes of EXT, A, and B inputs */
#define SR620_TSLP              "TSLP"  /* TSLP(?) i{,j} (0=pos,1=neg) */
#define SR620_TSLP_QUERY        "TSLP?"

/**
 ** Measurement Control Commands
 **/

#define SR620_TRG               "*TRG"  /* alias to STRT command */

#define SR620_ARMM              "ARMM"  /* ARMM(?) {j} (see below) */
#define SR620_ARMM_QUERY        "ARMM?"
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
#define SR620_AUTM              "AUTM"  /* AUTM(?) {j} (1=on, 0=off) */
#define SR620_AUTM_QUERY        "AUTM?"

/* change arming parity (equiv to toggling state of front panel COMPL LED) */
#define SR620_COMP              "COMP"

/* set/query display rel */
#define SR620_DREL              "DREL"/* DREL(?) j (0=clr,1=set,2=last,3=cur) */
#define SR620_DREL_QUERY        "DREL?"


/* set/query width of freq, period, or count gate to x (1ms - 500s) */
#define SR620_GATE              "GATE"  /* GATE(?) x (negative val = external)*/
#define SR620_GATE_QUERY        "GATE?"

/* set/query the type of jitter calculation */
#define SR620_JTTR              "JTTR"  /* JTTR(?) j (0=stddev,1=allanvar) */
#define SR620_JTTR_QUERY        "JTTR?"

/* set/query measurement mode */
#define SR620_MODE              "MODE"  /* MODE(?) j (see below) */
#define SR620_MODE_QUERY        "MODE?"
/* 0=time,1=width,2=rise/fall time,3=freq,4=period,5=phase,6=count */

#define SR620_SIZE              "SIZE"  /* SIZE(?) x */
#define SR620_SIZE_QUERY        "SIZE?"

#define SR620_SRCE              "SRCE"  /* SRCE(?) j (0=A,1=B,2=ref,3=A/B) */
#define SR620_SRCE_QUERY        "SRCE?"

#define SR620_STRT              "STRT"  /* START button (alias *TRG) */
#define SR620_STOP              "STOP"  /* RESET button */

/**
 ** Data Commands
 **/

#define SR620_MEAS              "MEAS" /* MEAS(?) j (0=mean,1=jit,2=max,3=min)*/
#define SR620_MEAS_QUERY        "MEAS?"

#define SR620_XALL_QUERY        "XALL?"
#define SR620_XAVG_QUERY        "XAVG?"
#define SR620_XJIT_QUERY        "XJIT?"
#define SR620_XMAX_QUERY        "XMAX?"
#define SR620_XMIN_QUERY        "XMIN?"

#define SR620_XREL              "XREL"  /* XREL(?) x */
#define SR620_XREL_QUERY        "XREL?"

#define SR620_XHST_QUERY        "XHST?" /* XHST? j (j=0 to 9) */
#define SR620_HSPT_QUERY        "HSPT?" /* HSPT? j (j=1 to 250) */
#define SR620_SCAV_QUERY        "SCAV?" /* SCAV? j (j=1 to 250) */

#define SR620_BDMP              "BDMP"  /* BDMP j (j=1 to 250) */

/**
 ** Scan Commands
 **/

#define SR620_ANMD              "ANMD"  /* ANMD(?) j */
#define SR620_ANMD_QUERY        "ANMD?"

#define SR620_DBEG              "DBEG"  /* DBEG(?) j */
#define SR620_DSEN              "DSEN"  /* DSEN(?) j */
#define SR620_DSTP              "DSTP"  /* DSTP(?) x */
#define SR620_HOLD              "HOLD"  /* HOLD(?) x */
#define SR620_SCAN              "SCAN"  /* SCAN */
#define SR620_SCEN              "SCEN"  /* SCEN(?) j */
#define SR620_SCLR              "SCLR"  /* SCLR */
#define SR620_SLOC_QUERY        "SLOC?"  /* SLOC? */
#define SR620_SCPT              "SCPT"  /* SCPT(?) j */
#define SR620_VBEG              "VBEG"  /* VBEG(?) j{,x} */
#define SR620_VOUT_QUERY        "VOUT?" /* VOUT? j */
#define SR620_VSTP              "VSTP"  /* VSTP(?) j{,x} */

/**
 ** Graphics Control Commands
 **/

/*
 * N.B.: it seems not possible to direct graph output to the
 * GPIB controller.  The SR620 wants to be the CIC when graphing to GPIB (?)
 */

#define SR620_AUTP              "AUTP"  /* AUTP(?) j */

#define SR620_AUTS              "AUTS"  /* AUTS */
#define SR620_CURS              "CURS"  /* CURS(?) j */
#define SR620_CURS_QUERY        "CURS?"
#define SR620_DGPH              "DGPH"  /* DGPH(?) j */
#define SR620_GCLR              "GCLR"  /* GCLR */
#define SR620_GENA              "GENA"  /* GENA(?) j */
#define SR620_GENA_QUERY        "GENA?"
#define SR620_GSCL              "GSCL"  /* GSCL(?) j{,x} */
#define SR620_GSCL_QUERY        "GSCL?"

#define SR620_PDEV              "PDEV"  /* PDEV(?) j (0=print, 1=plot) */
#define SR620_PLAD              "PLAD"  /* PLAD(?) j */
#define SR620_PLAD_QUERY        "PLAD?"
#define SR620_PLPT              "PLPT"  /* PLPT(?) j (0=rs232, 1=gpib) */
#define SR620_PLPT_QUERY        "PLPT?"
#define SR620_PLOT              "PLOT"  /* PLOT */
#define SR620_PCLR              "PCLR"  /* PCLR */

/**
 ** Front Panel Control
 **/

#define SR620_DISP              "DISP"  /* DISP(?) j */
#define SR620_KEYS              "KEYS"  /* KEYS(?) j */
#define SR620_EXPD              "EXPD"  /* EXPD(?) j */

/**
 ** Rear Panel Control
 **/

#define SR620_CLCK              "CLCK"  /* CLCK(?) j */
#define SR620_CLKF              "CLKF"  /* CLKF(?) j */
#define SR620_CLKF_QUERY        "CLKF?"
#define SR620_PORT              "PORT"  /* PORT(?) {j} */
#define SR620_PRTM              "PRTM"  /* PRTM(?) j */
#define SR620_RNGE              "RNGE"  /* RNGE(?) j{,k} */
#define SR620_VOLT_QUERY        "VOLT?" /* VOLT? j */

/**
 ** Interface Control Commands
 **/

#define SR620_RESET             "*RST"
#define SR620_ID_QUERY          "*IDN?" 
#define SR620_ID_RESPONSE       "StanfordResearchSystems,SR620" /* +sn,fwrev */

#define SR620_OPC               "*OPC" 
#define SR620_OPC_QUERY         "*OPC?" 
#define SR620_WAI               "*WAI"
#define SR620_ENDT              "ENDT"  /* ENDT {j,k,l,m} */
#define SR620_LOCL              "LOCL"  /* LOCL j */
#define SR620_WAIT              "WAIT"  /* WAIT(?) j */

/**
 ** Status Reporting Commands 
 **/

#define SR620_CLS               "*CLS"  /* clear all status registers */

/* set event status byte enable register */
#define SR620_ESR_ENABLE        "*ESE"  /* *ESE{?} j (j=value) */
#define SR620_ESR_ENABLE_QUERY  "*ESE?"

/* request contents of standard event status register */
#define SR620_ESR_QUERY         "*ESR?" /* *ESR? {j} (j=bit to request) */

#define SR620_PSC               "*PSC"  /* *PSC{?} j (0=no clr, 1=clr on pon) */
#define SR620_SRE               "*SRE"  /* *SRE(?) j */
#define SR620_STB_QUERY         "*STB?" /* *STB? {j} */

/* set error status enable register */
#define SR620_ERROR_ENABLE      "EREN"  /* EREN{?} j (j=value) */
#define SR620_ERROR_ENABLE_QUERY "EREN?"

/* request contents of error status byte - clears byte (or bit if specified) */
#define SR620_ERROR_QUERY       "ERRS?" /* ERRS? {j} (j=bit to request) */
#define SR620_STAT_QUERY        "STAT?" /* STAT? j */
#define SR620_TENA              "TENA"  /* TENA(?) j */
#define SR620_SETUP_QUERY       "STUP?"

/**
 ** Calibration Control (NOTE: not needed during normal operation)
 **/

/* $TAC? j */
/* $PHK(?) j */
/* $POT? j */
#define SR620_AUTOCAL           "*CAL?" 
#define SR620_SELFTEST          "*TST?"
/* BYTE(?) j{,k} */
/* WORD(?) j{,k} */

/**
 ** Bit definitions
 **/

/* serial poll status byte */
#define SR620_STATUS_READY      0x01    /* no measurements are in progress */
#define SR620_STATUS_PREADY     0x02    /* no prints are in progress */
#define SR620_STATUS_ERROR      0x04    /* unmasked bit set in error status */
#define SR620_STATUS_TIC        0x08    /* unmasked bit set in the TIC */
#define SR620_STATUS_MAV        0x10    /* gpib output queue is non-empty */    
#define SR620_STATUS_ESB        0x20    /* unmasked bit set in event status */
#define SR620_STATUS_RQS        0x40    /* SRQ (service request) bit */
#define SR620_STATUS_SREADY     0x80    /* no scans are in progress */

/* self test results */
#define SR620_TEST_ERRORS { \
    { 0,    "no error" }, \
    { 3,    "lost memory" }, \
    { 4,    "cpu error" }, \
    { 5,    "system ram error" }, \
    { 6,    "video ram error" }, \
    { 16,   "count gate error" }, \
    { 17,   "chan 2 count != 0" }, \
    { 18,   "chan 1 count error" }, \
    { 19,   "chan 1 count != 0" }, \
    { 20,   "chan 2 count error" }, \
    { 32,   "frequency gate error" }, \
    { 33,   "excessive jitter" }, \
    { 34,   "frequency insertion delay error" }, \
    { 35,   "time interval insertion delay error" }, \
    { 0,    NULL }, \
}

/* autocal results */
#define SR620_AUTOCAL_ERRORS { \
    { 0,    "no error" }, \
    { 1,    "not warmed up, can't calibrate" }, \
    { 2,    "no trigger error" }, \
    { 3,    "can't find edge on start tac" }, \
    { 4,    "can't find edge on start fine seek" }, \
    { 5,    "start tac calbyte out of range" }, \
    { 6,    "start tac non-convergence" }, \
    { 7,    "start linearity byte out of range" }, \
    { 19,   "can't find edge on stop tac" }, \
    { 20,   "can't find edge on stop fine seek" }, \
    { 21,   "stop tac calbyte out of range" }, \
    { 22,   "stop tac non-convergence" }, \
    { 23,   "stop linearity byte out of range" }, \
    { 0,    NULL }, \
}

/* standard event status register bits */
#define SR620_ESR_OPC           0x01    /* set by OPC when all ops complete */
#define SR620_ESR_QUERY_ERR     0x04    /* set on output queue overflow */
#define SR620_ESR_EXEC_ERR      0x10    /* set by an out of range parameter, */
                                        /*  or non-completion of some cmd */
                                        /*  due to a condition like overload */
#define SR620_ESR_CMD_ERR       0x20    /* set by a command syntax err, */
                                        /*  or unrecognized command */
#define SR620_ESR_CMD_URQ       0x40    /* set by any key press or trigger */
                                        /*  knob rotation */
#define SR620_ESR_CMD_PON       0x80    /* set by power on */

/* error status byte */
#define SR620_ERROR_PRINT       0x01
#define SR620_ERROR_NOCLOCK     0x02
#define SR620_ERROR_AUTOLEV_A   0x04
#define SR620_ERROR_AUTOLEV_B   0x08
#define SR620_ERROR_TEST        0x10
#define SR620_ERROR_CAL         0x20
#define SR620_ERROR_WARMUP      0x40
#define SR620_ERROR_OVFLDIV0    0x80

/* time interval counter status byte (read with */
#define SR620_TIC_TRIG_EXT      0x01
#define SR620_TIC_TRIG_A        0x02
#define SR620_TIC_TRIG_B        0x04
#define SR620_TIC_ARM           0x08
#define SR620_TIC_EXT_OVLD      0x10

                                            
/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
