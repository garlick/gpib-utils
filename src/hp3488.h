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

/* Information obtained from:
 * "HP 3488A Switch/Control Unit: Operating, Programming, and 
 * Configuration Manual", Sept 1, 1995.
 *
 * Notes:
 * <slot> is slot number 1-5 (1 digit)
 * <c> is channel address <slot><chan> (3 digits)
 */

#define HP3488_RESET        "RESET"	    /* reset all slots */
#define HP3488_TEST		    "TEST"	    /* execute self test */
#define HP3488_ID_QUERY     "ID?"       /* asks box for identity */
#define HP3488_ID_RESPONSE  "HP3488A"   /* response to above */
#define HP3488_STAT_QUERY   "STATUS"    /* read status byte */
#define HP3488_ERR_QUERY    "ERROR"     /* read error byte */
#define HP3488_MASK         "MASK"      /* set SRQ mask byte */
#define HP3488_OLAP_OFF     "OLAP 0"    /* commands are synchronous (default) */
#define HP3488_OLAP_ON      "OLAP 1"    /* commands can be interleaved */
#define HP3488_EHALT_OFF    "EHALT 0"   /* no stop on error (default) */
#define HP3488_EHALT_ON     "EHALT 1"   /* stop on error (lock gpib) */
#define HP3488_DISP         "DISP"      /* display string */
#define HP3488_DISP_ON      "DON"       /* display on (default) */
#define HP3488_DISP_OFF     "DOFF"      /* display off */
#define HP3488_LOCK_OFF     "LOCK 0"    /* no lock out keyboard (default) */
#define HP3488_LOCK_ON      "LOCK 1"    /* lock out keyboard */

#define HP3488_CTYPE        "CTYPE"	    /* CTYPE <slot> - query slot */
#define HP3488_CRESET       "CRESET"	/* CRESET <slot> - reset slot */
#define HP3488_CPAIR        "CPAIR"	    /* CPAIR <slot>,<slot> - pair slots */
#define HP3488_CMON         "CMON"	    /* CMON <slot> - monitor slot */
                            /* (0 cancels, -1*slot selects tracking mode) */

#define HP3488_DELAY_MS     "DELAY"     /* DELAY <time in ms> */
#define HP3488_CLOSE        "CLOSE"     /* CLOSE <c>[,<c>,...] - close chan */
#define HP3488_OPEN         "OPEN"      /* OPEN <c>[,<c>,...] - open chan */
#define HP3488_VIEW_QUERY   "VIEW"      /* VIEW <c> - view channel state */

/* For 44474A digital i/o assembly.
 */
#define HP3488_DWRITE       "DWRITE"    /* DWRITE <c>,<data>[,data>]... */
#define HP3488_DREAD        "DREAD"     /* DREAD <c>[,<# of times to read>] */
#define HP3488_DBW          "DBW"       /* DBW <c>,#I<block of data> */
#define HP3488_DBR          "DBR"       /* DBR <c>[,<# of times to read>] */
#define HP3488_DMODE        "DMODE"  /* DMODE <slot>[,<mode>][,<pol>][,<EI>] */

#define HP3488_DMODE_MODE_STATIC    1       /* default */
#define HP3488_DMODE_MODE_STATIC2   2       /* read what was written */
#define HP3488_DMODE_MODE_RWSTROBE  3       /* read & write strobe mode */
#define HP3488_DMODE_MODE_HANDSHAKE 4       /* handshake (no ext. inc. mode) */

#define HP3488_DMODE_POLARITY_LBP   0x01    /* lower byte polarity */
#define HP3488_DMODE_POLARITY_UBP   0x02    /* upper byte polarity */
#define HP3488_DMODE_POLARITY_PCTL  0x04    /* PCTL polarity (low ready) */
#define HP3488_DMODE_POLARITY_PFLG  0x08    /* PFLG polarity (low ready) */
#define HP3488_DMODE_POLARITY_IODIR 0x16    /* I/O direction line polarity */
                                            /*   high = input mode normally */

/* Status byte values
 * Also SRQ mask values (except rqs cannot be masked)
 */
#define HP3488_STATUS_SCAN_DONE     0x01    /* end of scan sequence */
#define HP3488_STATUS_OUTPUT_AVAIL  0x02    /* output available */
#define HP3488_STATUS_SRQ_POWER     0x04    /* power on SRQ asserted */
#define HP3488_STATUS_SRQ_BUTTON    0x08    /* front panel SRQ asserted */
#define HP3488_STATUS_READY         0x10    /* ready for instructions */
#define HP3488_STATUS_ERROR         0x20    /* an error has occurred */
#define HP3488_STATUS_RQS           0x40    /* rqs */

/* Error byte values
 */
#define HP3488_ERROR_SYNTAX         0x1     /* syntax error */
#define HP3488_ERROR_EXEC           0x2     /* exection error */
#define HP3488_ERROR_TOOFAST        0x4     /* hardware trigger too fast */
#define HP3488_ERROR_LOGIC          0x8     /* logic failure */
#define HP3488_ERROR_POWER          0x10    /* power supply failure */

/* Card model numbers, descriptions, and valid channel numbers.
 * Note: 44477 and 44476 are reported as 44471 by CTYPE command.
 */
#define MODELTAB    { \
    { 00000, "empty slot",            NULL }, \
    { 44470, "relay multiplexer",     "[00-09]" }, \
    { 44471, "general purpose relay", "[00-09]" }, \
    { 44472, "dual 4-1 VHF switch",   "[00-03,10-13]" }, \
    { 44473, "4x4 matrix switch",     "[00-03,10-13,20-23,30-33]" }, \
    { 44474, "digital I/O",           "[00-15]" }, \
    { 44475, "breadboard",            NULL }, \
    { 44476, "microwave switch",      "[00-03]" }, \
    { 44477, "form C relay",          "[00-06]" }, \
    { 44478, "1.3GHz multiplexer",    "[00-03,10-13]" }, \
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
