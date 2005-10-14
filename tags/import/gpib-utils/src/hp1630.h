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
 * HP Logic Analyzers Models 1631A/D Operation and Programming Manual
 * Logic Analyzer 1630A/D Operating Manual
 */

/*
 * Keyboard mnemonics.
 */
#define HP1630_KEY_SYSTEM_MENU      "SM" 
#define HP1630_KEY_FORMAT_MENU      "FM" 
#define HP1630_KEY_TRACE_MENU       "TM"
#define HP1630_KEY_LIST_MENU        "LM"
#define HP1630_KEY_WFORM_MENU       "WM"
#define HP1630_KEY_CHART_MENU       "CM" /* 1630 only? */

#define HP1630_KEY_CURSOR_LEFT      "CL"
#define HP1630_KEY_CURSOR_RIGHT     "CR"
#define HP1630_KEY_CURSOR_UP        "CU"
#define HP1630_KEY_CURSOR_DOWN      "CD"

#define HP1630_KEY_LABEL_LEFT       "LL"
#define HP1630_KEY_LABEL_RIGHT      "LR"
#define HP1630_KEY_LABEL_UP         "LU"
#define HP1630_KEY_LABEL_DOWN       "LD"

#define HP1630_KEY_ROLL_LEFT        "RL"
#define HP1630_KEY_ROLL_RIGHT       "RR"
#define HP1630_KEY_ROLL_UP          "RU"
#define HP1630_KEY_ROLL_DOWN        "RD"

#define HP1630_KEY_INSERT           "IN"
#define HP1630_KEY_DELETE           "DE"

#define HP1630_KEY_CLEAR_ENTRY      "CE"
#define HP1630_KEY_DEFAULT          "DM"

#define HP1630_KEY_NEXT             "NX"
#define HP1630_KEY_PREV             "PV"
#define HP1630_KEY_CHS              "CS"    /* 1630 only? */
#define HP1630_KEY_DONT_CARE        "DC"    /* 1630 only? */

#define HP1630_KEY_RUN              "RN"
#define HP1630_KEY_RESUME           "RE"
#define HP1630_KEY_STOP             "ST"
#define HP1630_KEY_PRINT            "PR"
#define HP1630_KEY_PRINT_ALL        "PA"

/*
 * Commamnds
 */
#define HP1630_CMD_BEEP             "BP"
#define HP1630_CMD_CURSOR_HOME      "CH"
#define HP1630_CMD_DISPLAY_BLANK    "DB"
#define HP1630_CMD_DISPLAY_READ     "DR" /* row col count */
#define HP1630_CMD_DISPLAY_WRITE    "DW" /* [i] row col "str" */
#define HP1630_CMD_SEND_IDENT       "ID"
#define HP1630_CMD_SEND_BUF_KEY     "KE"
#define HP1630_CMD_SET_SRQ_MASK     "MB" /* <mask> */
#define HP1630_CMD_POWER_UP         "PU"
#define HP1630_CMD_RESET            "RST" /* 1631 only? */
#define HP1630_CMD_STATUS_BYTE      "SB" /* <1-4> */

#define HP1630_CMD_TRANSMIT_CONFIG  "TC"
#define HP1630_CMD_TRANSMIT_STATE   "TS"
#define HP1630_CMD_TRANSMIT_TIMING  "TT"
#define HP1630_CMD_TRANSMIT_ANALOG  "TA" /* 1631 only */
#define HP1630_CMD_TRANSMIT_ALL     "TE"

#define HP1630_CMD_RECEIVE_CONFIG   "RC"
#define HP1630_CMD_RECEIVE_STATE    "RS"
#define HP1630_CMD_RECEIVE_TIMING   "RT"
#define HP1630_CMD_RECEIVE_ANALOG   "RA" /* 1631 only */
#define HP1630_CMD_RECEIVE_ALL      "RE"

/* 
 * Status byte 0 - only bits enabled by service request mask
 * Status byte 1 - all bits
 */
#define HP1630_STAT_PRINT_COMPLETE  0
#define HP1630_STAT_MEAS_COMPLETE   1
#define HP1630_STAT_SLOW_CLOCK      2
#define HP1630_STAT_KEY_PRESSED     3
#define HP1630_STAT_NOT_BUSY        4
#define HP1630_STAT_ERROR_LAST_CMD  5
#define HP1630_STAT_SRQ             6

/* 
 * Status byte 3 - trace status
 */
/* bits 4:7 */
#define HP1630_TRACE_NONE_TAKEN	    0
#define HP1630_TRACE_IN_PROGRESS    1
#define HP1630_TRACE_MEAS_COMPLETE  2
#define HP1630_TRACE_MEAS_ABORTED   4
/* bits 0:3 */
#define HP1630_TRACE_NOT_ACTIVE     0
#define HP1630_TRACE_WAIT_TRIGGER   1
#define HP1630_TRACE_WAIT_SEQ1      2
#define HP1630_TRACE_WAIT_SEQ2      3
#define HP1630_TRACE_WAIT_SEQ3      4
#define HP1630_TRACE_WAIT_OV_END    5
#define HP1630_TRACE_WAIT_OV_START  6
#define HP1630_TRACE_SLOW_CLOCK     7
#define HP1630_TRACE_TIMING_WAIT    8
#define HP1630_TRACE_TIMING_DELAYED 9
#define HP1630_TRACE_TIMING_IN_PROG 10

/* 
 * Status byte 4 - controller error codes
 */
#define HP1630_CERR_SUCCESS         0
#define HP1630_CERR_ILLEGAL_VALUE   4
#define HP1630_CERR_USE_NEXT_PREV   8
#define HP1630_CERR_NUM_ENTRY_REQ   9
#define HP1630_CERR_USE_HEX_KEYS    10
#define HP1630_CERR_USE_ALPHA_KEYS  11
#define HP1630_CERR_FIX_PROB_FIRST  13
#define HP1630_CERR_DONT_CARE_ILLEG 15
#define HP1630_CERR_USE_0_1         16
#define HP1630_CERR_USE_0_1_DC      17
#define HP1630_CERR_USE_0_7         18
#define HP1630_CERR_USE_0_7_DC      19
#define HP1630_CERR_USE_0_3         20
#define HP1630_CERR_USE_0_3_DC      21
#define HP1630_CERR_VAL_TOOBIG      22
#define HP1630_CERR_USE_CHS         24
#define HP1630_CERR_MAX_INS_USED    30
#define HP1630_CERR_CASSETTE_ERR    40
#define HP1630_CERR_BAD_CHECKSUM    41
#define HP1630_CERR_NO_CASSETTE     46
#define HP1630_CERR_ILLEG_FILENAME  47
#define HP1630_CERR_DUP_GPIB_ADDR   48
#define HP1630_CERR_RELOAD_DISC     49
#define HP1630_CERR_STORAGE_ABORTED 50
#define HP1630_CERR_FILE_NOT_FOUND  51
#define HP1630_CERR_BAD_COMMAND     58
#define HP1630_CERR_WP_DISC         60
#define HP1630_CERR_RESUME_NALLOW   61
#define HP1630_CERR_INVAL_IN_TRACE  62
#define HP1630_CERR_WRONG_REVISION  82

#define _bit_test(bit, mask)	(((mask)>>(bit)) & 1)

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
