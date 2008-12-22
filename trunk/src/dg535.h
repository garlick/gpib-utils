/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2005-2008 Jim Garlick <garlick.jim@gmail.com>

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

/* GPIB control codes for the Stanford Research Systems DG535
 * Digital Delay / Pulse Generator.
 */

/** 
 ** Initialization commands.
 **/

/* Clear instrument.  Clear comm buffers and recall default settings.
 * Default settings are:
 * trig:       single-shot (triggers off)
 * internal:   default trigger rate is 10KHz
 * burst mode: trigger rate is 10KHz, 10 pulses/burst, 20 periods/burst
 * external:   +1VDC, positive slope, hi-Z termination
 * delays:     all delays A, B, C, and D linked to T0 and set to zero
 * outputs:    all outputs are set to drive hi-Z loads to TTL levels
 * gpib:       gpib address not affected, line terminator is crlf with EOI
 */
#define DG535_CLEAR                     "CL"

/* Set one to three ASCII codes which will terminate each response
 * from the DG535.  EOI is always sent with the last char of the termination
 * sequence.
 */
#define DG535_GPIB_TERM                 "GT"       /* GT {i} {,j} {,k} */

/**
 ** Status commands
 **/

/* Return error status byte.
 * If integer value is present, return only that bit.
 */
#define DG535_ERROR_STATUS              "ES"       /* ES {i} */

/* Return the instrument status byte.
 * If integer value is present, return only that bit.
 */
#define DG535_INSTR_STATUS              "IS"       /* IS {i} */

/* Set status mask for SRQ to i.  Determines which status bits will
 * generate SRQ.
 */
#define DG535_STATUS_MASK               "SM"       /* SM {i} */

/* Set cursor mode (i=0) or number mode (i=1).
 */
#define DG535_CURSOR_MODE               "CS"       /* CS {i} */

/* Set cursor to column i=0 to 19
 */
#define DG535_CURSOR_SET                "SC"       /* SC {i} */

/* Move cursor left (i=0) or right (i=1)
 */
#define DG535_CURSOR_MOVE               "MC"       /* MC {i} */

/* Increment (i=1) or decrement (i=0) the digit at the current
 * cursor location.
 */
#define DG535_CURSOR_INCR               "IC"       /* IC {i} */

/* Display a string of 1-20 characters.
 */
#define DG535_DISPLAY_STRING            "DS"       /* DS string */

/**
 ** Delay and output commands.
 **/

/* Delay time of channel i is set to t seconds relative to channel j.
 */
#define DG535_DELAY_TIME                "DT"       /* DT i{,j,t} */

/* Set the termination impedence.  Output i is configured to drive a 
 * 50 ohm load (j=0) or high-Z load (j=1).
 */
#define DG535_TERM_Z                    "TZ"       /* TZ i{,j} */

/* Set output i to mode j, where mode is TTL (j=0), NIM (j=1), ECL (j=2),
 * or VARiable (j=3).
 */
#define DG535_OUT_MODE                  "OM"       /* OM i{,j} */

/* Amplitude of output i is set to v volts if in the VARiable mode.
 */
#define DG535_OUT_AMPL                  "OA"       /* OA i{,v} */

/* Output offset of output i is set to v volts if in VARiable mode.
 */
#define DG535_OUT_OFFSET                "OO"       /* OO i{,v} */

/* Output polarity of channel i is inverted (j=0) or normal (j=1)
 * for TTL, EC>L, or NIM outputs.
 */
#define DG535_OUT_POLARITY              "OP"       /* OP i{,j} */

/**
 ** Trigger commands
 **/

/* Set trigger mode to Int (i=0), Ext (i=1), SS (i=2), or Bur (i=3).
 */
#define DG535_TRIG_MODE                 "TM"       /* TM {i} */

/* Set internal trigger rate (i=0) or burst trigger rate (i=1) to f Hz.
 */
#define DG535_TRIG_RATE                 "TR"       /* TR i{,f} */

/* Set external trigger level to v Volts.
 */
#define DG535_TRIG_LEVEL                "TL"       /* TL {v} */

/* Set trigger slope to falling (i=0) or rising (i=1) edge.
 */
#define DG535_TRIG_SLOPE                "TS"       /* TS {i} */

/* Set the input impedance of the external trigger input to 50 ohms (j=0)
 * or high-Z (j=1).
 */
#define DG535_TRIG_Z                    "TZ"       /* TZ 0{,j} */

/* Single shot trigger if trigger mode = 2.
 */
#define DG535_SINGLE_SHOT               "SS"       /* SS */

/* Set burst count of i (2 thru 32766) pulses per burst.
 */
#define DG535_BURST_COUNT               "BC"       /* BC {i} */

/* Set burst period of i (4 to 32766) triggers per burst.
 */
#define DG535_BURST_PERIOD              "BP"       /* BP {i} */

/**
 ** Store and Recall commands
 **/

/* Store/recall all instrument settings to/from location i=1 to 9.
 */
#define DG535_STORE                     "ST"       /* ST {i} */
#define DG535_RECALL                    "RC"       /* RC {i} */

/**
 ** Bit encodings, etc.
 **/

/* Delay and output codes.
 */
#define DG535_DO_TRIG                   0       /* trigger input */
#define DG535_DO_T0                     1       /* T0 output */
#define DG535_DO_A                      2       /* A output */
#define DG535_DO_B                      3       /* B output */
#define DG535_DO_AB                     4       /* AB and -AB outputs */
#define DG535_DO_C                      5       /* C output */
#define DG535_DO_D                      6       /* D output */
#define DG535_DO_CD                     7       /* CD and -CD output */

#define DG535_OUT_NAMES { \
    { DG535_DO_T0, "t0" }, \
    { DG535_DO_A,  "a" }, \
    { DG535_DO_B,  "b" }, \
    { DG535_DO_AB, "ab" }, \
    { DG535_DO_C,  "c" }, \
    { DG535_DO_D,  "d" }, \
    { DG535_DO_CD, "cd" }, \
    { 0, NULL } \
}

/* Output modes.
 */
#define DG535_OUT_TTL                   0
#define DG535_OUT_NIM                   1
#define DG535_OUT_ECL                   2
#define DG535_OUT_VAR                   3

#define DG535_OUT_MODES { \
    { DG535_OUT_TTL, "ttl" }, \
    { DG535_OUT_NIM, "nim" }, \
    { DG535_OUT_ECL, "ecl" }, \
    { DG535_OUT_VAR, "var" }, \
    { 0, NULL } \
}

/* Trigger modes.
 */
#define DG535_TRIG_INT                  0
#define DG535_TRIG_EXT                  1
#define DG535_TRIG_SS                   2
#define DG535_TRIG_BUR                  3

#define DG535_TRIG_MODES { \
    { DG535_TRIG_INT, "int" }, \
    { DG535_TRIG_EXT, "ext" }, \
    { DG535_TRIG_SS,  "ss" }, \
    { DG535_TRIG_BUR, "bur" }, \
    { 0, NULL } \
}

/* Error status byte values.
 */
#define DG535_ERR_RECALL                0x40    /* recalled data was corrupt */
#define DG535_ERR_DELAY_RANGE           0x20    /* delay range error */
#define DG535_ERR_DELAY_LINKAGE         0x10    /* delay linkage error */
#define DG535_ERR_CMDMODE               0x08    /* wrong mode for command */
#define DG535_ERR_ERANGE                0x04    /* value out of range */
#define DG535_ERR_NUMPARAM              0x02    /* wrong number of paramters */
#define DG535_ERR_BADCMD                0x01    /* unrecognized command */

/* Instrument status byte values.
 * These bits can generate SRQ if present in the status mask.
 */
#define DG535_STAT_BADMEM               0x40    /* memory contents corrupted */
#define DG535_STAT_SRQ                  0x20    /* service request */
#define DG535_STAT_TOOFAST              0x10    /* trigger rate too high */
#define DG535_STAT_PLL                  0x08    /* 80MHz PLL is unlocked */
#define DG535_STAT_TRIG                 0x04    /* trigger has occurred */
#define DG535_STAT_BUSY                 0x02    /* busy with timing cycle */
#define DG535_STAT_CMDERR               0x01    /* command error detected */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
