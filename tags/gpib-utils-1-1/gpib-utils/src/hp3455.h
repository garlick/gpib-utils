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

/*
 * HP3455 program codes
 */
#define HP3455_FUNC_DC		"F1"
#define HP3455_FUNC_AC		"F2"
#define HP3455_FUNC_FAST_AC	"F3"
#define HP3455_FUNC_2WIRE_KOHMS	"F4"
#define HP3455_FUNC_4WIRE_KOHMS	"F5"
#define HP3455_FUNC_TEST	"F6"

#define HP3455_RANGE_0_1	"R1"
#define HP3455_RANGE_1		"R2"
#define HP3455_RANGE_10		"R3"
#define HP3455_RANGE_100	"R4"
#define HP3455_RANGE_1K		"R5"
#define HP3455_RANGE_10K	"R6"
#define HP3455_RANGE_AUTO	"R7"

#define HP3455_TRIGGER_INT	"T1"
#define HP3455_TRIGGER_EXT	"T2"
#define HP3455_TRIGGER_HOLD_MAN	"T3"

#define HP3455_MATH_SCALE	"M1"
#define HP3455_MATH_ERROR	"M2"
#define HP3455_MATH_OFF		"M3"

#define HP3455_ENTER_Y		"EY"
#define HP3455_ENTER_Z		"EZ"

#define HP3455_STORE_Y		"SY"
#define HP3455_STORE_Z		"SZ"

#define HP3455_AUTOCAL_OFF	"A0"
#define HP3455_AUTOCAL_ON	"A1"

#define HP3455_HIGHRES_OFF	"H0"
#define HP3455_HIGHRES_ON	"H1"

#define HP3455_DATA_RDY_RQS_OFF	"D0"
#define HP3455_DATA_RDY_RQS_ON	"D1"

#define HP3455_BINARY_PROGRAM	"B"

/* 
 * HP3455 status byte bits (1=true, 0=false)
 */
#define HP3455_STAT_DATA_READY	0x1
#define HP3455_STAT_SYNTAX_ERR	0x2
#define HP3455_STAT_BINARY_ERR	0x4
#define HP3455_STAT_TOOFAST_ERR	0x8

#define HP3455_STAT_SRQ         0x40

