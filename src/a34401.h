/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2007 Jim Garlick <garlick.jim@gmail.com>

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

/* Status byte values
 */
#define A34401_STAT_RQS  64      /* device is requesting service */
#define A34401_STAT_SER  32      /* bits are set in std. event register */
#define A34401_STAT_MAV  16      /* message(s) available in the output buffer */
#define A34401_STAT_QDR  8       /* bits are set in quest. data register */

/* Standard event register values.
 */
#define A34401_SER_PON   128     /* power on */
#define A34401_SER_CME   32      /* command error */
#define A34401_SER_EXE   16      /* execution error */
#define A34401_SER_DDE   8       /* device dependent error */
#define A34401_SER_QYE   4       /* query error */
#define A34401_SER_OPC   1       /* operation complete */

/* Questionable data register.
 */
#define A34401_QDR_LFH   4096    /* limit fail HI */
#define A34401_QDR_LFL   2048    /* limit fail LO */
#define A34401_QDR_ROL   512     /* ohms overload */
#define A34401_QDR_AOL   2       /* current overload */
#define A34401_QDR_VOL   1       /* voltage overload */


/* SCPI commands for this instrument are mnemonic (and plentiful!) enough 
 * that we do not need to go nuts defining them here.
 */

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
