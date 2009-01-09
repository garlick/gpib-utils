/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2005-2009 Jim Garlick <garlick.jim@gmail.com>

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

/* Keyboard mnemonics.
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

/* Commamnds
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

/* Status byte 0 - only bits enabled by service request mask
 * Status byte 1 - all bits
 */
#define HP1630_STAT_PRINT_COMPLETE  0x01 
#define HP1630_STAT_MEAS_COMPLETE   0x02 
#define HP1630_STAT_SLOW_CLOCK      0x04
#define HP1630_STAT_KEY_PRESSED     0x08
#define HP1630_STAT_NOT_BUSY        0x10 
#define HP1630_STAT_ERROR_LAST_CMD  0x20
#define HP1630_STAT_SRQ             0x40

/* Status byte 3 (bits 4:7) - trace status
 */
#define SB3Q2_ERRORS {                                  \
    { 0,    "no trace taken" },                         \
    { 1,    "trace in progress" },                      \
    { 2,    "measurement complete" },                   \
    { 3,    "measurement aborted" },                    \
    { 0,    NULL },                                     \
}

/* Status byte 3 (bits 0:3) - trace status
 */
#define SB3Q1_ERRORS {                                  \
    { 0,    "not active (no measurement in progress" }, \
    { 1,    "waiting for state trigger term" },         \
    { 2,    "waiting for sequence term #1" },           \
    { 3,    "waiting for sequence term #2" },           \
    { 4,    "waiting for sequence term #3" },           \
    { 5,    "waiting for time interval end term" },     \
    { 6,    "waiting for time interval start term" },   \
    { 7,    "slow clock" },                             \
    { 8,    "waiting for timing trigger" },             \
    { 9,    "delaying timing trace" },                  \
    { 10,   "timing trace in progress" },               \
    { 0,    NULL },                                     \
}

/* Status byte 4 - controller error codes
 */
#define SB4_ERRORS {                                    \
    { 0,    "success" },                                \
    { 4,    "illegal value" },                          \
    { 8,    "use NEXT[] PREV[] keys" },                 \
    { 9,    "numeric entry required" },                 \
    { 10,   "use hex keys" },                           \
    { 11,   "use alphanumeric keys" },                  \
    { 13,   "fix problem first" },                      \
    { 15,   "DONT CARE not legal here" },               \
    { 16,   "use 0 or 1" },                             \
    { 17,   "use 0, 1, or DONT CARE" },                 \
    { 18,   "use 0 thru 7" },                           \
    { 19,   "use 0 thru 7 or DONT CARE" },              \
    { 20,   "use 0 thru 3" },                           \
    { 21,   "use 0 thru 3 or DONT CARE" },              \
    { 22,   "value is too large" },                     \
    { 24,   "use CHS key" },                            \
    { 30,   "maximum INSERTs used" },                   \
    { 40,   "cassette contains non HP163x data" },      \
    { 41,   "checksum does not match" },                \
    { 46,   "no cassette in drive" },                   \
    { 47,   "illegal filename" },                       \
    { 48,   "duplicate GPIB address" },                 \
    { 49,   "reload disc first" },                      \
    { 50,   "storage operation aborted" },              \
    { 51,   "file not found" },                         \
    { 58,   "unrecognized command" },                   \
    { 60,   "write protected disc" },                   \
    { 61,   "RESUME not allowed" },                     \
    { 62,   "invalid in this trace mode" },             \
    { 82,   "incorrect revision code" },                \
    { 0,    NULL },                                     \
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
