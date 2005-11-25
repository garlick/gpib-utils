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
 * Commands
 */

/* Put the DMM into a known state:
 * G0 - internal reference
 * T1 - internal trigger
 * N0 - disable NULL
 * P0 - disable percent 
 * L0 - disable LAH (low average high)
 * S0 - disable time
 */
#define R5005_CMD_INIT			"Z"


#define R5005_CMD_TERM			"X"
#define R5005_CMD_ERROR_XMIT		"Y"

#define R5005_FUNC_DCV			"F1"
#define R5005_FUNC_ACV			"F2"
#define R5005_FUNC_KOHMS		"F3" /* or F4 */

#define R5005_RANGE_AUTO		"R0"
#define R5005_RANGE_0_1			"R3"
#define R5005_RANGE_1			"R4"
#define R5005_RANGE_10			"R5"
#define R5005_RANGE_100			"R6"
#define R5005_RANGE_1K			"R7"
#define R5005_RANGE_10K			"R8"

#define R5005_TRIGGER_IMMEDIATE		"T0"
#define R5005_TRIGGER_INT		"T1"
#define R5005_TRIGGER_EXT		"T2"
#define R5005_TRIGGER_HOLD		"T3"
#define R5005_TRIGGER_EXT_DELAY		"T4"
#define R5005_TRIGGER_HOLD_DELAY	"T5"

#define R5005_CAL_ZERO			"K2" /* K0, K1 ignored */
#define R5005_CAL_OFF_DEC		"K3"
#define R5005_CAL_OFF_NOM		"K4"
#define R5005_CAL_OFF_INC		"K4"
#define R5005_CAL_SCALE_DEC		"K5"
#define R5005_CAL_SCALE_NOM		"K6"
#define R5005_CAL_SCALE_INC		"K7"
#define R5005_CAL_CHECKSUM		"K8"

#define R5005_NULL_DISABLE		"N0"
#define R5005_NULL_ENABLE		"N1"
#define R5005_NULL_STORE		"N2"
#define R5005_NULL_XMIT			"N3"

#define R5005_PCT_DISABLE		"P0"
#define R5005_PCT_ENABLE		"P1"
#define R5005_PCT_STORE			"P2"
#define R5005_PCT_XMIT			"P3"

#define R5005_LAH_DISABLE		"L0"
#define R5005_LAH_ENABLE_READING	"L1"
#define R5005_LAH_ENABLE_LOW		"L2"
#define R5005_LAH_ENABLE_AVG		"L3"
#define R5005_LAH_ENABLE_HIGH		"L4"
#define R5005_LAH_ENABLE_NUM		"L5"
#define R5005_LAH_STORE			"L6"
#define R5005_LAH_XMIT			"L7"

#define R5005_PGM_STORE			"A" /* [0-9] */
#define R5005_PGM_RECALL		"B" /* [0-9] */

#define R5005_DAT_CLEAR			"C0"
#define R5005_DAT_XMIT			"C1"

#define R5005_REF_INTERNAL		"G0"
#define R5005_REF_DC			"G1"
#define R5005_REF_AC			"G2"

#define R5005_REFRANGE_AUTO		"Q0"
#define R5005_REFRANGE_0_1		"Q3"
#define R5005_REFRANGE_1		"Q4"
#define R5005_REFRANGE_10		"Q5"
#define R5005_REFRANGE_100		"Q6"
#define R5005_REFRANGE_1K		"Q7"
#define R5005_REFRANGE_10K		"Q8"

#define R5005_TIME_DISABLE		"S0"
#define R5005_TIME_ENABLE		"S1"
#define R5005_TIME_STORE_START		"S2"
#define R5005_TIME_STORE_STOP		"S3"
#define R5005_TIME_STORE_INTERVAL	"S4"
#define R5005_TIME_STORE_SUBINTERVAL	"S5"
#define R5005_TIME_STORE_N		"S6"
#define R5005_TIME_STORE_PRESENT	"S7"
#define R5005_TIME_XMIT			"S8"

#define R5005_HIGHRES_OFF		"I1"
#define R5005_HIGHRES_ON		"I2" /* or I3 */

#define R5005_FILT_OUT			"J0"
#define R5005_FILT_IN			"J1"

#define R5005_SRQ_NONE			"D0"
#define R5005_SRQ_READY			"D1"
#define R5005_SRQ_LT_ZERO		"D2"
#define R5005_SRQ_GE_ZERO		"D3"

#define R5005_INPUT_SIGNAL		"V0"
#define R5005_INPUT_REFERENCE		"V1"

/*
 * Errors
 */
#define R5005_ERR_PERCENT_CONST		0
#define R5005_ERR_ZERO_NOT_DC_0_1	1
#define R5005_ERR_ZERO_NOSHORT   	2
#define R5005_ERR_BADRAM_U35		3
#define R5005_ERR_NVRAM			4
#define R5005_ERR_ZERO_BIGOFFSET	5
#define R5005_ERR_PERCENT_DEVIATION	6
#define R5005_ERR_BADRAM_U22_U31	7
#define R5005_ERR_STORE 		8
#define R5005_ERR_RECALL		9
#define R5005_ERR_RECALL_EMPTY		10
#define R5005_ERR_TRIGGER_TOO_FAST	11
#define R5005_ERR_SYNTAX		12
#define R5005_ERR_OPTION		13

/*
 * Status byte bits.
 */
#define R5005_STAT_DATA_READY		0x01
#define R5005_STAT_ERROR_AVAIL		0x08
#define R5005_STAT_SIG_INTEGRATE	0x10
#define R5005_STAT_ABNORMAL		0x20
#define R5005_STAT_SRQ			0x40
