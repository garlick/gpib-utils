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

/* References: 
 * HP Logic Analyzers Models 1631A/D Operation and Programming Manual
 * Logic Analyzer 1630A/D Operating Manual
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#if HAVE_GETOPT_LONG
#include <getopt.h>
#endif
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>

#include "util.h"
#include "gpib.h"
#include "hpcrc.h"

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

static int _interpret_status(gd_t gd, unsigned char status, char *msg);
static void _setdate(gd_t gd);
static void _verify_model(gd_t gd);
static void _restore(gd_t gd, int dryrun);
static void _save(gd_t gd, char *cmd);
static void _printscreen(gd_t gd, int allflag);

char *prog;

/* value of status byte 4 */
static strtab_t _sb4_errors[] = SB4_ERRORS;

#define INSTRUMENT "hp1630"
#define PATH_DATE  "/bin/date"

#define MAXCONFBUF (16*1024+256)
#define MAXPRINTBUF (128*1024)
#define MAXMODELBUF (16)

static const char *options = OPTIONS_COMMON "lcpPsCSTArdV";

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    OPTIONS_COMMON_LONG,
    {"local",           no_argument, 0, 'l'},
    {"clear",           no_argument, 0, 'c'},
    {"print-screen",    no_argument, 0, 'p'},
    {"print-all",       no_argument, 0, 'P'},
    {"save-all",        no_argument, 0, 's'},
    {"save-config",     no_argument, 0, 'C'},
    {"save-state",      no_argument, 0, 'S'},
    {"save-timing",     no_argument, 0, 'T'},
    {"save-analog",     no_argument, 0, 'A'},
    {"restore",         no_argument, 0, 'r'},
    {"verify",          no_argument, 0, 'V'},
    {"date",            no_argument, 0, 'd'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static opt_desc_t optdesc[] = {
    OPTIONS_COMMON_DESC,
    { "l", "local",         "return instrument to front panel controle" },
    { "c", "clear",         "set default values, verify model no., set date" },
    { "p", "print-screen",  "print screen to stdout" },
    { "P", "print-all",     "print all to stdout" },
    { "s", "save-all",      "save complete analyzer state to stdout" },
    { "C", "save-config",   "save config to stdout" },
    { "S", "save-state",    "save state acquisition data to stdout" },
    { "T", "save-timing",   "save timing acquisition data to stdout" },
    { "A", "save-analog",   "save analog data to stdout (1631 only)" },
    { "r", "restore",       "restore analyzer state from stdin" },
    { "V", "verify",        "verify checksums of analyzer state from stdin" },
    { "d", "date",          "set date"},
    { 0, 0, 0},
};

int
main(int argc, char *argv[])
{
    int c, print_usage = 0;
    gd_t gd;

    gd = gpib_init_args(argc, argv, options, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage)
        usage(optdesc);
    if (!gd)
        exit(1);

    /* set the errors _interpret_status() will see */
    gpib_wrtf(gd, "MB %d\r\n", HP1630_STAT_ERROR_LAST_CMD);

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            OPTIONS_COMMON_SWITCH
                break;
            case 'l':   /* --local */
                gpib_loc(gd);
                break;
            case 'c':   /* --clear */
                gpib_clr(gd, 1000000);
                gpib_wrtf(gd, "PU\r\n");
                _verify_model(gd);
                _setdate(gd);
                break;
            case 'p':   /* --print-screen */
                _printscreen(gd, 0);
                break;
            case 'P':   /* --print-all */
                _printscreen(gd, 1);
                break;
            case 's':   /* --save-all */
                _save(gd, "TE");
                break;
            case 'C':   /* --save-config */
                _save(gd, "TC");
                break;
            case 'S':   /* --save-state */
                _save(gd, "TS");
                break;
            case 'T':   /* --save-timing */
                _save(gd, "TT");
                break;
            case 'A':   /* --save-analog (1631 only) */
                _save(gd, "TA");
                break;
            case 'r':   /* --restore */
                _restore(gd, 0);
                break;
            case 'V':   /* --verify */
                _restore(gd, 1);
                break;
            case 'd':   /* --date */
                _setdate(gd);
                break;
        }
    }

    gpib_fini(gd);

    exit(0);
}

/* Interpet serial poll results (status byte)
 *   Return: 0=non-fatal/no error, >0=fatal, -1=retry.
 */
static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

    if ((status & HP1630_STAT_SRQ)) {
        if (status & HP1630_STAT_ERROR_LAST_CMD) {
            fprintf(stderr, "%s: srq: problem with last command\n", prog);
            err = 1;
        }
    }
    return err;
}

/* Return true if buffer begins with HP graphics magic.
 */
static int
_hpgraphics(uint8_t *buf, int len)
{
    return (len > 2 && buf[0] == '\033' && buf[1] == '*');
}

/* Send analyzer screen dump to stdout.  Output may be in hp bitmap format or
 * it may be text with hp escape sequences to indicate bold, etc...
 */
static void
_printscreen(gd_t gd, int allflag)
{
    uint8_t buf[MAXPRINTBUF];
    int count;

    if (allflag)
        gpib_wrtf(gd, "PA\r\n");
    else
        gpib_wrtf(gd, "PR\r\n");
    count = gpib_rd(gd, buf, sizeof(buf));
    fprintf(stderr, "%s: Print %s: %d bytes (%s format)\n", prog,
            allflag ? "all" : "screen", count,
            _hpgraphics(buf, count) ? "HP graphics" : "HP text");
    if (write_all(1, buf, count) < 0) {
        perror("write");
        exit(1);
    }
}

static void
_getunixdate(int *month, char *datestr, int len)
{
    FILE *pipe;
    char cmd[80];

    if (snprintf(cmd, sizeof(cmd), "%s +'%%m%%d%%Y%%H%%M%%S'", PATH_DATE) < 0) {
        fprintf(stderr, "%s: out of memory", prog);
        exit(1);
    }
    if (!(pipe = popen(cmd, "r"))) {
        perror("popen");
        exit(1);
    }
    if (fscanf(pipe, "%2d%s", month, datestr) != 2) {
        fprintf(stderr, "%s: error parsing date output\n", prog);
        exit(1);
    }
    assert(strlen(datestr) < len);
    if (pclose(pipe) < 0) {
        perror("pclose");
        exit(1);
    }
}

/* Set the date on the analyzer to match the UNIX date.
 */
static void
_setdate(gd_t gd)
{
    char datestr[64];
    int month;
    int i;

    _getunixdate(&month, datestr, 64); /* get date from UNIX */

    /* Position cursor on month, clear entry to reset month to Jan,
     * then hit 'next' enough times to advance to the desired month.
     */
    gpib_wrtf(gd, "SM;NX;CD;CE\r\n");
    for (i = 1; i < month; i++)       /* 1 = jan, 2 = feb, etc. */
        gpib_wrtf(gd, "NX\r\n");
    /* position over day and spew forth the remaining digits */
    gpib_wrtf(gd, "CR;\"%s\"\r\n", datestr);
}

static void
_verify_model(gd_t gd)
{
    char *supported[] = { "HP1630A", "HP1630D", "HP1630G",
                          "HP1631A", "HP1631D", NULL };
    char buf[MAXMODELBUF];
    int found = 0;
    int i;

    gpib_wrtf(gd, "ID\r\n");
    gpib_rdstr(gd, buf, sizeof(buf));
    for (i = 0; supported[i] != NULL; i++)
        if (!strncmp(buf, supported[i], strlen(supported[i])))
            found = 1;
    if (!found)
        fprintf(stderr, "%s: warning: model %s is unsupported\n", prog, buf);
}

/* Return a string representation of learn string command,
 * or NULL if invalid.
 */
static char * 
_ls_cmd(uint8_t *ls)
{
    if (!strncmp((char *)&ls[0], "RC", 2))
        return "Config";
    else if (!strncmp((char *)&ls[0], "RS", 2))
        return "State ";
    else if (!strncmp((char *)&ls[0], "RT", 2))
        return "Timing";
    else if (!strncmp((char *)&ls[0], "RA", 2))
        return "Analog";
    else if (!strncmp((char *)&ls[0], "XX", 2))
        return "InvAsm";
    return NULL;
}

static int
_ls_ignorecrc(uint8_t *ls)
{
    /* HP 10391B inverse assembler files have uninitialized(?) CRC */
    if (!strncmp((char *)&ls[0], "XX", 2))
        return 1;
    return 0;
}

/* Return the learn string length.
 * Includes data + CRC, not cmd + length.
 */
static uint16_t
_ls_len(uint8_t *ls)
{
    return ((uint16_t)(ls[2] << 8) + ls[3]);
}

/* Return the learn string CRC.
 */
static uint16_t
_ls_crc(uint8_t *ls)
{
    uint16_t len = _ls_len(ls) + 4; /* including cmd + length */

    return ((uint16_t)(ls[len - 2] << 8) + ls[len - 1]);
}

static int
_ls_parse(uint8_t *ls, int rawlen)
{
    char *cmd;
    uint16_t len;

    if (rawlen < 4 || (len = _ls_len(ls)) + 4 > rawlen) {
        fprintf(stderr, "%s: truncated learn string\n", prog);
        exit(1);
    }
    if (!(cmd = _ls_cmd(ls))) {
        fprintf(stderr, "%s: corrupt learn string header\n", prog);
        exit(1);
    }
    if (!_ls_ignorecrc(ls) && _ls_crc(ls) != hpcrc(ls+4, len-2)) {
        fprintf(stderr, "%s: learn string CRC does not match\n", prog);
        exit(1);
    }

    fprintf(stderr, "%s: %s: %-4.4d+4 bytes\n", prog, cmd, len);

    return len + 4;
}

static void
_restore(gd_t gd, int dryrun)
{
    int len, i;
    uint8_t buf[MAXCONFBUF];

    len = read_all(0, buf, sizeof(buf));
    if (len < 0) {
        perror("read");
        exit(1);
    }

    /* Iterate through concatenated learn strings:
     * parsing, verifying, and writing to analyzer.
     */
    i = 0;
    while (i < len) {
        int lslen; 
        uint8_t status;
        int count;

        lslen = _ls_parse(buf + i, len - i);

        if (!dryrun) {
            /* write the learn string command */
            gpib_wrt(gd, buf + i, lslen);
            /* pad LS with magic some f/w revisions need [Adam Goldman] */
            gpib_wrtf(gd,"\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");

            /* get status byte 4 and check for error */
            gpib_wrtf(gd, "SB 4\r\n");
            count = gpib_rd(gd, &status, 1);
            if (count != 1) {
                fprintf(stderr, "%s: error reading status byte 4\n", prog);
                exit(1);
            }
            if (status != 0)
                fprintf(stderr, "%s: %s\n", prog, findstr(_sb4_errors, status));
        }

        i += lslen;
    }
}

/* NOTE: if analyzer is not configured with any state lines (e.g. analog only
 * on a 1631), save all and save state will return 'unrecognized command'.
 */
static void
_save(gd_t gd, char *cmd)
{
    uint8_t buf[MAXCONFBUF];
    int count, len;
    uint8_t status;
    int i;

    gpib_wrtf(gd, "%s\r\n", cmd);
    len = gpib_rd(gd, buf, sizeof(buf));

    if (write_all(1, buf, len) < 0) {
        perror("write");
        exit(1);
    }

    /* get status byte 4 and check for error */
    gpib_wrtf(gd, "SB 4\r\n");
    count = gpib_rd(gd, &status, 1);
    if (count != 1) {
        fprintf(stderr, "%s: error reading status byte 4\n", prog);
        exit(1);
    }
    if (status != 0)
        fprintf(stderr, "%s: %s\n", prog, findstr(_sb4_errors, status) );

    /* parse what was read */
    i = 0;
    while (i < len)
        i += _ls_parse(buf + i, len - i);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
