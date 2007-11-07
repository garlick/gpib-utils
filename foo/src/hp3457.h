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

/* Misc functions.
 */
#define HP3457_TONE          "TONE"        /* beep */
#define HP3457_ID_QUERY      "ID?"         /* request instrument id */
#define HP3457_ID_RESPONSE   "HP3457A"     /*   response to id query */
#define HP3457_CALNUM_QUERY  "CALNUM?"     /* request # calibrations */
#define HP3457_DISP          "DISP"        /* DISP {on|off|msg}{,message}*/
#define HP3457_INBUF         "INBUF"       /* INBUF {on|off} */
#define HP3457_LFREQ         "LFREQ"       /* LFREQ {50|60} */
#define HP3457_LFREQ_QUERY   "LFREQ?"      /* query line freq */
#define HP3457_LINE_QUERY    "LINE?"       /* query measured line freq */
/* Lock keyboard.
 */
#define HP3457_LOCK          "LOCK"        /* LOCK {on|off} */

/* Init for local use (issue GPIB device clear first):
 * ACBAND 20; AZERO ON; CRESET; DCV AUTO; DELAY -1; DISP ON; END OFF
 * FIXEDZ OFF; FSOURCE ACV; INBUF OFF; LOCK OFF; MATH OFF,OFF; all math
 * regs set to 0 except DEGREE=20, SCALE=1, PERC=1, REF=1, RES=50;
 * MEM OFF; MFORMAT SREAL; NDIG 5; NPLC 10; NRDGS 1,AUTO; OCOMP OFF;
 * OFORMAT ASCII; SADV HOLD; SLIST (empty list); TARM AUTO; TERM FRONT;
 * TIMER 1; TRIG AUTO.
 */
#define HP3457_RESET         "RESET"

/* Init for remote use (issue GPIB device clear first):
 * ACBAND 20; AZERO ON; BEEP ON; CRESET; DCV AUTO; DELAY -1; DISP ON;
 * FIXEDZ OFF; F SOURCE ACV; INBUF OFF; LOCK OFF; MATH OFF,OFF; all math
 * regs set to 0 except DEGREE=20, PERC=1, REF=1, RES=50, SCALE=1; 
 * MEM OFF; MFORMAT SREAL; NDIG 5; NPLC 1; NRDGS 1,AUTO; OCOMP OFF;
 * OFORMAT ASCII; SADV HOLD; SLIST (empty list); TARM AUTO; TERM FRONT;
 * TIMER 1; TRIG SYN.
 */
#define HP3457_PRESET        "PRESET"

/* Select function.
 */
#define HP3457_DCV           "FUNC DCV"    /* DC volts */
#define HP3457_ACV           "FUNC ACV"    /* AC volts */
#define HP3457_ACDCV         "FUNC ACDCV"  /* ACDC volts */
#define HP3457_2WIRE_OHMS    "FUNC OHM"    /* ohms (2 wire) */
#define HP3457_4WIRE_OHMS    "FUNC OHMF"   /* ohms (4 wire) */
#define HP3457_DCI           "FUNC DCI"    /* DC current */
#define HP3457_ACI           "FUNC ACI"    /* AC current */
#define HP3457_ACDCI         "FUNC ACDCI"  /* ACDC current */
#define HP3457_FREQ          "FUNC FREQ"   /* frequency */
#define HP3457_PERIOD        "FUNC PER"    /* period */

/* Select trigger mode.
 */
#define HP3457_TRIG_AUTO     "TRIG AUTO"   /* T. on not busy */
#define HP3457_TRIG_EXT      "TRIG EXT"    /* T. on ext input */
#define HP3457_TRIG_SGL      "TRIG SGL"    /* T. once then hold */
#define HP3457_TRIG_HOLD     "TRIG HOLD"   /* stop T. */
#define HP3457_TRIG_SYN      "TRIG SYN"    /* T. on data read */

#define HP3457_TRIG_QUERY    "TRIG?"       /* query trigger mode */
                                           /* 1=auto 2=ext 4=hold 5=syn */

/* Select/request range.
 */
#define HP3457_RANGE         "RANGE"       /* RANGE {max input}{,% res} */
#define HP3457_RANGE_QUERY   "RANGE?"      /* request current range */

/* Select fast/slow AC voltage measurement based on expected frequency (Hz).
 * Slow mode is <400Hz, fast mode is >= 400 Hz.
 */
#define HP3457_ACBAND        "ACBAND"      /* ACBAND {freq} */

/* Set autozero mode.  With AZERO on, instrument takes a zero
 * reading before every measurement, doubling measurement time.
 */
#define HP3457_AZERO         "AZERO"       /* AZERO {on|off} */

/* Set fixed impedance mode for DCV.
 * If set, instrument maintains 10mohm impedeance for all ranges.
 */
#define HP3457_FIXEDZ        "FIXEDZ"      /* FIXEDZ {on|off} */
#define HP3457_FIXEDZ_QUERY  "FIXEDZ?"     /* request fixedz 0=off,1=on */

/* Set source for freq measurement.
 */
#define HP3457_FSOURCE       "FSOURCE"     /* FSOURCE {acv|acdcv|aci|acdci} } */

/* Math operations.
 */
#define HP3457_MATH_OFF      "MATH OFF"    /* math off */
#define HP3457_MATH_CONT     "MATH CONT"   /* resume prev. math op. */
#define HP3457_MATH_CTHRM    "MATH CTHRM"  /* ohms->thermistor (C) */
#define HP3457_MATH_DB       "MATH DB"     /* db conversion */
#define HP3457_MATH_DBM      "MATH DBM"    /* dbm conversion */
#define HP3457_MATH_FILTER   "MATH FILTER" /* low pass filter */
#define HP3457_MATH_FTHRM    "MATH FTHRM"  /* ohms->thermistor (F) */
#define HP3457_MATH_NULL     "MATH NULL"   /* r-offset */
#define HP3457_MATH_PERC     "MATH PERC"   /* ((r-perc)/perc)*100 */
#define HP3457_MATH_PFAIL    "MATH PFAIL"  /* reading vs max and min */
#define HP3457_MATH_RMS      "MATH RMS"    /* */
#define HP3457_MATH_SCALE    "MATH SCALE"  /* */
#define HP3457_MATH_STAT     "MATH STAT"   /* */

#define HP3457_MATH_QUERY    "MATH?"
#define HP3457_MCOUNT        "MCOUNT?"



/* Perform automatic calibrations and store result in NVRAM.
 * Input signals should be disconnected.
 */
#define HP3457_ACAL_BOTH     "ACAL ALL"    /* AC+OHMS cal (takes 35s) */
#define HP3457_ACAL_AC       "ACAL AC"     /* AC cal (takes 3s) */
#define HP3457_ACAL_OHMS     "ACAL OHMS"   /* OHMS cal (takes 32s) */

/* Perform self test.
 */
#define HP3457_TEST          "TEST"

#define HP3457_NDIG3         "NDIG 3"
#define HP3457_NDIG4         "NDIG 4"
#define HP3457_NDIG5         "NDIG 5"
#define HP3457_NDIG6         "NDIG 6"

/* Status/error/aux byte operations.
 */
#define HP3457_REQUEST_SERVICE "RQS"       /* set status bits to cause SRQ */
#define HP3457_CSB           "CSB"         /* clear status byte */
#define HP3457_ERR_QUERY     "ERR?"        /* request error register */
#define HP3457_AUXERR_QUERY  "AUXERR?"     /* request aux error register */

/* Set which err bits cause status bit 7 to be set. 
 */
#define HP3457_EMASK         "EMASK"       /* EMASK {mask} */


/* Status byte bits.
 */
#define HP3457_STAT_EXEC_DONE  0x01        /* program memory exec completed */
#define HP3457_STAT_HILO_LIMIT 0x02        /* hi or lo limited exceeded */
#define HP3457_STAT_SRQ_BUTTON 0x04        /* front panel SRQ key pressed */
#define HP3457_STAT_SRQ_POWER  0x08        /* power-on SRQ occurred */
#define HP3457_STAT_READY      0x10        /* ready */
#define HP3457_STAT_ERROR      0x20        /* error (consult error reg) */
#define HP3457_STAT_SRQ        0x40        /* service requested */

/* Error register bits.
 */
#define HP3457_ERR_HARDWARE    0x0001      /* hw error (consult aux error reg) */
#define HP3457_ERR_CAL_ACAL    0x0002      /* error in CAL or ACAL process */
#define HP3457_ERR_TOOFAST     0x0004      /* trigger too fast */
#define HP3457_ERR_SYNTAX      0x0008      /* syntax error */
#define HP3457_ERR_COMMAND     0x0010      /* unknown command received */
#define HP3457_ERR_PARAMETER   0x0020      /* unknown parameter received */
#define HP3457_ERR_RANGE       0x0040      /* parameter out of range */
#define HP3457_ERR_MISSING     0x0080      /* required parameter missing */
#define HP3457_ERR_IGNORED     0x0100      /* parameter ignored */
#define HP3457_ERR_CALIBRATION 0x0200      /* out of calibration */
#define HP3457_ERR_AUTOCAL     0x0400      /* autocal required */

/* Aux error register bits.
 */
#define HP3457_AUXERR_OISO     0x0001      /* isolation error during operation */
                                           /*   in any mode (self-test, */
                                           /*   autocal, measurements, etc) */
#define HP3457_AUXERR_SLAVE    0x0002      /* slave processor self-test failure */
#define HP3457_AUXERR_TISO     0x0004      /* isolation self-test failure */
#define HP3457_AUXERR_ICONV    0x0008      /* integrator convergence error */
#define HP3457_AUXERR_ZERO     0x0010      /* front end zero meas error */
#define HP3457_AUXERR_IGAINDIV 0x0020      /* current src, gain sel, div fail */
#define HP3457_AUXERR_AMPS     0x0040      /* amps self-test failure */
#define HP3457_AUXERR_ACAMP    0x0080      /* ac amp dc offset failure */
#define HP3457_AUXERR_ACFLAT   0x0100      /* ac flatness check */
#define HP3457_AUXERR_OHMPC    0x0200      /* ohms precharge fail in autocal */
#define HP3457_AUXERR_32KROM   0x0400      /* 32K ROM checksum failure */
#define HP3457_AUXERR_8KROM    0x0800      /* 8K ROM checksum failure */
#define HP3457_AUXERR_NVRAM    0x1000      /* NVRAM failure */
#define HP3457_AUXERR_RAM      0x2000      /* volatile RAM failure */
#define HP3457_AUXERR_CALRAM   0x4000      /* cal RAM protection failure */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
