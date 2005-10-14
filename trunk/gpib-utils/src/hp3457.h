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

#define HP3457_TONE		"TONE"		/* beep */
#define HP3457_ID		"ID?"		/* send instrument id */
#define HP3457_RESET		"RESET"		/* init for front panel use */
#define HP3457_PRESET		"PRESET"	/* init for remote use */

/* Max input parameter follows, e.g. "DCV .03".
 * Alternate method: "FUNC DCV; RANGE .03"
 */
#define HP3457_DCV		"DCV"
#define HP3457_ACV		"ACV"
#define HP3457_ACDCV		"ACDCV"
#define HP3457_2WIRE_OHMS	"OHM"
#define HP3457_4WIRE_OHMS	"OHMF"
#define HP3457_DCI		"DCI"
#define HP3457_ACI		"ACI"
#define HP3457_ACDCI		"ACDCI"
#define HP3457_FREQ		"FREQ"
#define HP3457_PERIOD 		"PER"

#define HP3457_TRIGGER_AUTO	"TRIG 1" /* triggers when not busy */
#define HP3457_TRIGGER_EXT	"TRIG 2" /* triggers on ext input */
#define HP3457_TRIGGER_SGL	"TRIG 3" /* triggers once (upon receipt of  */
					 /*   TRIG SGL) then HOLDs */
#define HP3457_TRIGGER_HOLD	"TRIG 4" /* stops triggering */
#define HP3457_TRIGGER_SYN	"TRIG 5" /* triggers when output buf empty, */
					 /*   reading memory is off/empty, */
					 /*   and controller requests data */

#define HP3457_MATH_SCALE	"M1"
#define HP3457_MATH_ERROR	"M2"
#define HP3457_MATH_OFF		"M3"

#define HP3457_ENTER_Y		"EY"
#define HP3457_ENTER_Z		"EZ"

#define HP3457_STORE_Y		"SY"
#define HP3457_STORE_Z		"SZ"

#define HP3457_ACAL_BOTH	"ACAL 1"
#define HP3457_ACAL_AC		"ACAL 2"
#define HP3457_ACAL_OHMS	"ACAL 3"

#define HP3457_TEST		"TEST"	/* execute self test */

#define HP3457_NDIG3		"NDIG 3"
#define HP3457_NDIG4		"NDIG 4"
#define HP3457_NDIG5		"NDIG 5"
#define HP3457_NDIG6		"NDIG 6"

#define HP3457_REQUEST_SERVICE	"RQS" 	/* set status bits to cause SRQ */
#define HP3457_CSB		"CSB"	/* clear status byte */
#define HP3457_ERR		"ERR?"	/* request error register */
#define HP3457_AUXERR		"AUXERR?" /* request aux error register */

/* 
 * HP3457 status byte bits
 */
#define HP3457_STAT_EXEC_DONE	1  	/* program memory exec completed */
#define HP3457_STAT_HILO_LIMIT	2  	/* hi or lo limited exceeded */
#define HP3457_STAT_SRQ_BUTTON	4	/* front panel SRQ key pressed */
#define HP3457_STAT_SRQ_POWER	8	/* power-on SRQ occurred */
#define HP3457_STAT_READY	16	/* ready */
#define HP3457_STAT_ERROR	32	/* error (consult error reg) */
#define HP3457_STAT_SRQ		64	/* service requested */

/*
 * HP3457 error register bits
 */
#define HP3457_ERR_HARDWARE	1	/* hw error (consult aux error reg) */
#define HP3457_ERR_CAL_ACAL	2	/* error in CAL or ACAL process */
#define HP3457_ERR_TOOFAST	4	/* trigger too fast */
#define HP3457_ERR_SYNTAX	8	/* syntax error */
#define HP3457_ERR_COMMAND	16	/* unknown command received */
#define HP3457_ERR_PARAMETER	32	/* unknown parameter received */
#define HP3457_ERR_RANGE	64	/* parameter out of range */
#define HP3457_ERR_MISSING	128	/* required parameter missing */
#define HP3457_ERR_IGNORED	256	/* parameter ignored */
#define HP3457_ERR_CALIBRATION	512	/* out of calibration */
#define HP3457_ERR_AUTOCAL	1024	/* autocal required */

/*
 * HP3457 aux error register bits
 */
#define HP3457_AUXERR_OISO	1	/* isolation error during operation */
					/*   in any mode (self-test, */
					/*   autocal, measurements, etc) */
#define HP3457_AUXERR_SLAVE	2	/* slave processor self-test failure */
#define HP3457_AUXERR_TISO	4	/* isolation self-test failure */
#define HP3457_AUXERR_ICONV	8	/* integrator convergence error */
#define HP3457_AUXERR_ZERO	16	/* front end zero meas error */
#define HP3457_AUXERR_IGAINDIV	32	/* current src, gain sel, div fail */
#define HP3457_AUXERR_AMPS	64	/* amps self-test failure */
#define HP3457_AUXERR_ACAMP	128	/* ac amp dc offset failure */
#define HP3457_AUXERR_ACFLAT	256 	/* ac flatness check */
#define HP3457_AUXERR_OHMPC	512	/* ohms precharge fail in autocal */
#define HP3457_AUXERR_32KROM	1024	/* 32K ROM checksum failure */
#define HP3457_AUXERR_8KROM	2048	/* 8K ROM checksum failure */
#define HP3457_AUXERR_NVRAM	4096	/* NVRAM failure */
#define HP3457_AUXERR_RAM	8192	/* volatile RAM failure */
#define HP3457_AUXERR_CALRAM	16384	/* cal RAM protection failure */
