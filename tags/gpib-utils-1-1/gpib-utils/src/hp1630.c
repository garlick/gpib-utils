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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include <assert.h>
#include <stdarg.h>
#include <gpib/ib.h>

#include "hp1630.h"

static char *prog;

typedef struct {
    int     num;
    char   *str;
} errstr_t;

#define INSTRUMENT "hp1630" /* the /etc/gpib.conf entry */
#define PATH_DATE  "/bin/date"


#define OPTIONS "pPn:bdRsrcAta"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"print-screen",    no_argument, 0, 'p'},
    {"print-all",       no_argument, 0, 'P'},
    {"beep",            no_argument, 0, 'b'},
    {"date",            no_argument, 0, 'd'},
    {"reset",           no_argument, 0, 'R'},
    {"save-all",        no_argument, 0, 's'},
    {"save-config",     no_argument, 0, 'c'},
    {"save-state",      no_argument, 0, 'A'},
    {"save-timing",     no_argument, 0, 't'},
    {"save-analog",     no_argument, 0, 'a'},
    {"restore",         no_argument, 0, 'r'},
    {0, 0, 0, 0},
};

static void 
usage(void)
{
    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name     name of instrument [1631a]\n"
"  -b,--beep          make the analyzer beep\n"
"  -p,--print-screen  print screen to stdout\n"
"  -P,--print-all     print all to stdout\n"
"  -d,--date          set analyzer date/time\n"
"  -R,--reset         default all instrument menus to power-up values\n"
"  -s,--save-all      save complete analyzer state to stdout\n"
"  -c,--save-config   save config to stdout\n"
"  -A,--save-state    save state acquisition data to stdout\n"
"  -t,--save-timing   save timing acquisition data to stdout\n"
"  -a,--save-analog   save analog data to stdout (1631 only)\n"
"  -r,--restore       restore analyzer state (partial or complete) from stdin\n"
           , prog);
    exit(1);
}

/* value of status byte 4 */
static errstr_t _sb4_errors[] = {
    { HP1630_CERR_SUCCESS,          "success" },
    { HP1630_CERR_ILLEGAL_VALUE,    "illegal value" },
    { HP1630_CERR_USE_NEXT_PREV,    "use NEXT[] PREV[] keys" },
    { HP1630_CERR_NUM_ENTRY_REQ,    "numeric entry required" },
    { HP1630_CERR_USE_HEX_KEYS,     "use hex keys" },
    { HP1630_CERR_USE_ALPHA_KEYS,   "use alphanumeric keys" },
    { HP1630_CERR_FIX_PROB_FIRST,   "fix problem first" },
    { HP1630_CERR_DONT_CARE_ILLEG,  "DONT CARE not legal here" },
    { HP1630_CERR_USE_0_1,          "use 0 or 1" },
    { HP1630_CERR_USE_0_1_DC,       "use 0, 1, or DONT CARE" },
    { HP1630_CERR_USE_0_7,          "use 0 thru 7" },
    { HP1630_CERR_USE_0_7_DC,       "use 0 thru 7 or DONT CARE" },
    { HP1630_CERR_USE_0_3,          "use 0 thru 3" },
    { HP1630_CERR_USE_0_3_DC,       "use 0 thru 3 or DONT CARE" },
    { HP1630_CERR_VAL_TOOBIG,       "value is too large" },
    { HP1630_CERR_USE_CHS,          "use CHS key" },
    { HP1630_CERR_MAX_INS_USED,     "maximum INSERTs used" },
    { HP1630_CERR_CASSETTE_ERR,     "cassette contains non HP163x data" },
    { HP1630_CERR_BAD_CHECKSUM,     "checksum does not match" },
    { HP1630_CERR_NO_CASSETTE,      "no cassette in drive" },
    { HP1630_CERR_ILLEG_FILENAME,   "illegal filename" },
    { HP1630_CERR_DUP_GPIB_ADDR,    "duplicate GPIB address" },
    { HP1630_CERR_RELOAD_DISC,      "reload disc first" },
    { HP1630_CERR_STORAGE_ABORTED,  "storage operation aborted" },
    { HP1630_CERR_FILE_NOT_FOUND,   "file not found" },
    { HP1630_CERR_BAD_COMMAND,      "unrecognized command" },
    { HP1630_CERR_WP_DISC,          "write protected disc" },
    { HP1630_CERR_RESUME_NALLOW,    "RESUME not allowed" },
    { HP1630_CERR_INVAL_IN_TRACE,   "invalid in this trace mode" },
    { HP1630_CERR_WRONG_REVISION,   "incorrect revision code" },
    { 0, NULL },
};

static char *_finderr(errstr_t *tab, int num)
{
    errstr_t *tp;

    for (tp = &tab[0]; tp->str != NULL; tp++)
        if (tp->num == num)
            break;

    return tp->str ? tp->str : "unknown error";
}

/* Warning: ibrsp has the side effect of modifying ibcnt! */
static void _checksrq(int d)
{
    char status;

    ibrsp(d, &status);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibrsp error %d\n", prog, iberr);
        exit(1);
    }
    if ((status & HP1630_STAT_SRQ)) {
        if (status & HP1630_STAT_ERROR_LAST_CMD) {
            fprintf(stderr, "%s: srq: syntax error\n", prog);
            exit(1);
        }
    }
}

static void _ibwrt(int d, char *str, int len)
{
    ibwrt(d, str, len);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibwrt timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibwrt error %d\n", prog, iberr);
        exit(1);
    }
    if (ibcnt != len) {
        fprintf(stderr, "%s: ibwrt: short write\n", prog);
        exit(1);
    }
    _checksrq(d);
}

static void _ibwrtstr(int d, char *str)
{
    _ibwrt(d, str, strlen(str));
}

static void _ibwrtf(int d, char *fmt, ...)
{
        va_list ap;
        char *s;
        int n;

        va_start(ap, fmt);
        n = vasprintf(&s, fmt, ap);
        va_end(ap);
        if (n == -1) {
            fprintf(stderr, "%s: out of memory\n", prog);
            exit(1);
        }
        _ibwrtstr(d, s);
        free(s);
}

static int _ibrd(int d, char *buf, int len)
{
    int count;

    ibrd(d, buf, len);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibrd timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibrd error %d\n", prog, iberr);
        exit(1);
    }
    assert(ibcnt >=0 && ibcnt <= len);
    count = ibcnt;

     _checksrq(d);

    return count;
}

static void _ibrdstr(int d, char *buf, int len)
{
    int count;
    
    count = _ibrd(d, buf, len - 1);
    assert(count < len);
    buf[count] = '\0';
}


static void _ibloc(int d)
{
    ibloc(d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibloc error %d\n", prog, iberr);
        exit(1);
    }
    _checksrq(d);
}

#if 0
static void _ibclr(int d)
{
    ibclr(d);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibclr timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibclr error %d\n", prog, iberr);
        exit(1);
    }
    _checksrq(d);
}
#endif

/*
 * Send analyzer screen dump to stdout.  Output may be in hp bitmap format or
 * it may be text with hp escape sequences to indicate bold, etc...
 */
static void
_printscreen(int device, int allflag)
{
    char buf[128*1024];
    int count;

    if (allflag)
        _ibwrtf(device, "%s\r\n", HP1630_KEY_PRINT_ALL);
    else
        _ibwrtf(device, "%s\r\n", HP1630_KEY_PRINT);
    count = _ibrd(device, buf, sizeof(buf));
    if (fwrite(buf, count, 1, stdout) != 1) {
        fprintf(stderr, "%s: error writing to stdout\n", prog);
        exit(1);
    }
}

static void
_getdate(int *month, char *datestr, int len)
{
	FILE *pipe;
    char *cmd;

    if (asprintf(&cmd, "%s +'%%m%%d%%Y%%H%%M%%S'", PATH_DATE) < 0) {
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
    free(cmd);
}

/*
 * Set the date on the analyzer to match the UNIX date.
 */
static void
_setdate(int device)
{
    char datestr[64];
    int month;
	int i;

    _getdate(&month, datestr, 64); /* get date from UNIX */

	/* Position cursor on month and hit 'next' enough times to 
	 * advance from jan to the desired month.
 	 * FIXME: assumes that the analyzer was just powered on and is set to
	 * [Jan] 00 0000 00:00:00.
	 */
	_ibwrtf(device, "%s;%s;%s\r\n", HP1630_KEY_SYSTEM_MENU, 
                    HP1630_KEY_NEXT, HP1630_KEY_CURSOR_DOWN);
	for (i = 1; i < month; i++)     /* 1 = jan, 2 = feb, etc. */
		_ibwrtf(device, "%s\r\n", HP1630_KEY_NEXT);
	/* position over day and spew forth the remaining digits */
    _ibwrtf(device, "%s;\"%s\"\r\n", HP1630_KEY_CURSOR_RIGHT, datestr);
}

static void
_verify_model(int device)
{
    char *supported[] = { "HP1630A", "HP1630D", "HP1630G",
                          "HP1631A", "HP1631D", NULL };
    char buf[32];
    int found = 0;
    int i;

	_ibwrtf(device, "%s\r\n", HP1630_CMD_SEND_IDENT);
	_ibrdstr(device, buf, sizeof(buf));
    for (i = 0; supported[i] != NULL; i++)
        if (!strcmp(buf, supported[i]))
            found = 1;

    if (!found)
        fprintf(stderr, "%s: warning: model %s is unsupported\n", prog, buf);
}

/* NOTE: if analyzer is not configured with any state lines (e.g. analog only
 * on a 1631), save all and save state will return 'unrecognized command'.
 */
static void
_savestate(int device, char *cmd)
{
    unsigned char buf[16*1024];
    int count;
    char status;

    _ibwrtf(device, "%s\r\n", cmd);
    count = _ibrd(device, buf, sizeof(buf));

    if (fwrite(buf, count, 1, stdout) != 1) {
        fprintf(stderr, "%s: error writing to stdout\n", prog);
        exit(1);
    }

    _ibwrtf(device, "%s 4\r\n", HP1630_CMD_STATUS_BYTE); /* ask for sb 4 */
    count = _ibrd(device, &status, 1);
    if (count != 1) {
        fprintf(stderr, "%s: error reading status byte 4\n", prog);
        exit(1);
    }
    fprintf(stderr, "%s: %s\n", prog, _finderr(_sb4_errors, status) );
}

/* Translate two-char restore command to string */
static char * 
_restorecmd(char *cmd)
{
    if (!strncmp(cmd, "RC", 2))
        return "Config";
    else if (!strncmp(cmd, "RS", 2))
        return "State ";
    else if (!strncmp(cmd, "RT", 2))
        return "Timing";
    else if (!strncmp(cmd, "RA", 2))
        return "Analog";
    return NULL;
}

static int
_restorelen(unsigned char *len)
{
    return ((len[0] << 8) + len[1]);
}

static void
_restorestate(int device)
{
    int c;
    unsigned char buf[16*1024];
    unsigned char *p = &buf[0], *q = &buf[0];
    int count;
    char status;

    while ((p - buf < sizeof(buf)) && (c = getchar()) != EOF)
        *p++ = c;
    if (p - buf < 4) {
        fprintf(stderr, "%s: short read\n", prog);
        exit(1);;
    }
    do {
        char *str = _restorecmd(q);
        int count = _restorelen(q+2);

        if (str == NULL) {
            fprintf(stderr, "%s: corrupt state file (byte %d)\n", prog,
                    q - buf);
            exit(1);
        }
        fprintf(stderr, "%s: restoring %s: %-4.4d+4 bytes\n", 
                prog, str, count);
        q += (count + 4);
    } while (q < p-6); /* 6 bytes of overhead in each segment */

	_ibwrt(device, buf, p - buf);

    _ibwrtf(device, "%s 4\r\n", HP1630_CMD_STATUS_BYTE); /* ask for sb 4 */
    count = _ibrd(device, &status, 1);
    if (count != 1) {
        fprintf(stderr, "%s: error reading status byte 4\n", prog);
        exit(1);
    }
    fprintf(stderr, "%s: %s\n", prog, _finderr(_sb4_errors, status) );
}


int
main(int argc, char *argv[])
{
    enum { OPT_NONE, OPT_PRINT_ALL, OPT_PRINT_SCREEN, OPT_BEEP,
           OPT_DATE, OPT_RESET, OPT_SAVE_ALL,
           OPT_SAVE_STATE, OPT_SAVE_TIMING, OPT_SAVE_CONFIG, OPT_SAVE_ANALOG,
           OPT_RESTORE, } opt = OPT_NONE;
    char *instrument = INSTRUMENT;
    int device, c;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'n':
            instrument = optarg;
            break;
        case 'p': opt = OPT_PRINT_SCREEN; break;
        case 'P': opt = OPT_PRINT_ALL; break;
        case 'b': opt = OPT_BEEP; break;
        case 'd': opt = OPT_DATE; break;
        case 'R': opt = OPT_RESET; break;
        case 's': opt = OPT_SAVE_ALL; break;
        case 'c': opt = OPT_SAVE_CONFIG; break;
        case 'A': opt = OPT_SAVE_STATE; break;
        case 't': opt = OPT_SAVE_TIMING; break;
        case 'a': opt = OPT_SAVE_ANALOG; break;
        case 'r': opt = OPT_RESTORE; break;
        }
    }

    if (opt == OPT_NONE)
        usage();

    /*
     * Initialize gpib and verify it's an hp163x analyzer.
     */
    device = ibfind(instrument);
    if (device < 0) {
        fprintf(stderr, "%s: not found: %s\n", prog, instrument);
        exit(1);
    } 

#if 0
    {
        /* doesn't lose configuration */
        _ibclr(device);
        /* XXX this would appear to be a bad thing - it times out reading the
         * status byte, and if the status byte read is taken out, it times out
         * on the next operation.
         */
    }
#else
    { /* do clear out any pending SRQ's if we're not resetting though */
        char status;

        ibrsp(device, &status);
        /* ignore result */
    }
#endif

    /* set the errors _checksrq() will see */
    _ibwrtf(device, "%s %d\r\n", HP1630_CMD_SET_SRQ_MASK,
            HP1630_STAT_ERROR_LAST_CMD);

    _verify_model(device);

    /*
     * Perform desired actions.
     */
    switch (opt) {

    /* print */
    case OPT_PRINT_SCREEN:
        _printscreen(device, 0);
        break;
    case OPT_PRINT_ALL:
        _printscreen(device, 1);
        break;

    /* configuration/test */
    case OPT_BEEP:
        _ibwrtf(device, "%s\r\n", HP1630_CMD_BEEP);
        break;
    case OPT_DATE:
        _setdate(device);
        break;
    case OPT_RESET:
        _ibwrtf(device, "%s\r\n", HP1630_CMD_POWER_UP);
        break;
    /* save/restore state */
    case OPT_SAVE_ALL:
        _savestate(device, HP1630_CMD_TRANSMIT_ALL);
        break;
    case OPT_SAVE_CONFIG:
        _savestate(device, HP1630_CMD_TRANSMIT_CONFIG);
        break;
    case OPT_SAVE_STATE:
        _savestate(device, HP1630_CMD_TRANSMIT_STATE);
        break;
    case OPT_SAVE_TIMING:
        _savestate(device, HP1630_CMD_TRANSMIT_TIMING);
        break;
    case OPT_SAVE_ANALOG: /* 1631 only */
        _savestate(device, HP1630_CMD_TRANSMIT_ANALOG);
        break;
    case OPT_RESTORE:
        _restorestate(device);
        break;
    case OPT_NONE:
        usage();
        /*NOTREACHED*/
    }

    _ibloc(device);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
