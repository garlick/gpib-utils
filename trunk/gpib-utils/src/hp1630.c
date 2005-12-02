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
#include <stdint.h>

#include "errstr.h"
#include "gpib.h"
#include "hp1630.h"

static char *prog;

/* value of status byte 4 */
static errstr_t _sb4_errors[] = SB4_ERRORS;

#define INSTRUMENT "hp1630" /* the /etc/gpib.conf entry */
#define PATH_DATE  "/bin/date"

#define MAXCONFBUF (16*1024)
#define MAXPRINTBUF (128*1024)
#define MAXMODELBUF (16)

#define OPTIONS "n:clvpPbsCAtar"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument, 0, 'c'},
    {"local",           no_argument, 0, 'l'},
    {"verbose",         no_argument, 0, 'v'},
    {"print-screen",    no_argument, 0, 'p'},
    {"print-all",       no_argument, 0, 'P'},
    {"beep",            no_argument, 0, 'b'},
    {"save-all",        no_argument, 0, 's'},
    {"save-config",     no_argument, 0, 'C'},
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
"  -n,--name name     name of instrument [%s]\n"
"  -c,--clear         set default values, verify model no., set date\n"
"  -l,--local         return instrument to local operation on exit\n"
"  -v,--verbose       show protocol on stderr\n"
"  -p,--print-screen  print screen to stdout\n"
"  -P,--print-all     print all to stdout\n"
"  -s,--save-all      save complete analyzer state to stdout\n"
"  -C,--save-config   save config to stdout\n"
"  -A,--save-state    save state acquisition data to stdout\n"
"  -t,--save-timing   save timing acquisition data to stdout\n"
"  -a,--save-analog   save analog data to stdout (1631 only)\n"
"  -r,--restore       restore analyzer state (partial or complete) from stdin\n"
           , prog, INSTRUMENT);
    exit(1);
}

static void 
hp1630_checksrq(int d)
{
    char status;

    gpib_ibrsp(d, &status); /* NOTE: modifies ibcnt */

    if ((status & HP1630_STAT_SRQ)) {
        if (status & HP1630_STAT_ERROR_LAST_CMD) {
            fprintf(stderr, "%s: srq: syntax error\n", prog);
            exit(1);
        }
    }
}

/*
 * Send analyzer screen dump to stdout.  Output may be in hp bitmap format or
 * it may be text with hp escape sequences to indicate bold, etc...
 */
static void
hp1630_printscreen(int d, int allflag)
{
    uint8_t buf[MAXPRINTBUF];
    int count;

    if (allflag)
        gpib_ibwrtf(d, "%s\r\n", HP1630_KEY_PRINT_ALL);
    else
        gpib_ibwrtf(d, "%s\r\n", HP1630_KEY_PRINT);
    hp1630_checksrq(d);
    count = gpib_ibrd(d, buf, sizeof(buf));
    hp1630_checksrq(d);
    if (fwrite(buf, count, 1, stdout) != 1) {
        fprintf(stderr, "%s: error writing to stdout\n", prog);
        exit(1);
    }
    hp1630_checksrq(d);
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
hp1630_setdate(int d)
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
	gpib_ibwrtf(d, "%s;%s;%s\r\n", HP1630_KEY_SYSTEM_MENU, 
                    HP1630_KEY_NEXT, HP1630_KEY_CURSOR_DOWN);
    hp1630_checksrq(d);
	for (i = 1; i < month; i++) {     /* 1 = jan, 2 = feb, etc. */
		gpib_ibwrtf(d, "%s\r\n", HP1630_KEY_NEXT);
        hp1630_checksrq(d);
    }
	/* position over day and spew forth the remaining digits */
    gpib_ibwrtf(d, "%s;\"%s\"\r\n", HP1630_KEY_CURSOR_RIGHT, datestr);
    hp1630_checksrq(d);
}

static void
hp1630_verify_model(int d)
{
    char *supported[] = { "HP1630A", "HP1630D", "HP1630G",
                          "HP1631A", "HP1631D", NULL };
    char buf[MAXMODELBUF];
    int found = 0;
    int i;

	gpib_ibwrtf(d, "%s\r\n", HP1630_CMD_SEND_IDENT);
    hp1630_checksrq(d);
	gpib_ibrdstr(d, buf, sizeof(buf));
    hp1630_checksrq(d);
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
hp1630_savestate(int d, char *cmd)
{
    uint8_t buf[MAXCONFBUF];
    int count;
    uint8_t status;

    gpib_ibwrtf(d, "%s\r\n", cmd);
    hp1630_checksrq(d);
    count = gpib_ibrd(d, buf, sizeof(buf));
    hp1630_checksrq(d);

    if (fwrite(buf, count, 1, stdout) != 1) {
        fprintf(stderr, "%s: error writing to stdout\n", prog);
        exit(1);
    }

    gpib_ibwrtf(d, "%s 4\r\n", HP1630_CMD_STATUS_BYTE); /* ask for sb 4 */
    hp1630_checksrq(d);
    count = gpib_ibrd(d, &status, 1);
    hp1630_checksrq(d);
    if (count != 1) {
        fprintf(stderr, "%s: error reading status byte 4\n", prog);
        exit(1);
    }
    fprintf(stderr, "%s: %s\n", prog, finderr(_sb4_errors, status) );
}

/* Translate two-char restore command to string */
static char * 
_restorecmd(uint8_t *cmd)
{
    if (!strncmp((char *)cmd, "RC", 2))
        return "Config";
    else if (!strncmp((char *)cmd, "RS", 2))
        return "State ";
    else if (!strncmp((char *)cmd, "RT", 2))
        return "Timing";
    else if (!strncmp((char *)cmd, "RA", 2))
        return "Analog";
    return NULL;
}

static int
_restorelen(uint8_t *len)
{
    return ((int)(len[0] << 8) + len[1]);
}

static void
hp1630_restorestate(int d)
{
    int c;
    uint8_t buf[MAXCONFBUF];
    uint8_t *p = &buf[0], *q = &buf[0];
    int count;
    uint8_t status;

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

	gpib_ibwrt(d, buf, p - buf);
    hp1630_checksrq(d);

    gpib_ibwrtf(d, "%s 4\r\n", HP1630_CMD_STATUS_BYTE); /* ask for sb 4 */
    hp1630_checksrq(d);
    count = gpib_ibrd(d, &status, 1);
    hp1630_checksrq(d);
    if (count != 1) {
        fprintf(stderr, "%s: error reading status byte 4\n", prog);
        exit(1);
    }
    fprintf(stderr, "%s: %s\n", prog, finderr(_sb4_errors, status) );
}

int
main(int argc, char *argv[])
{
    enum { OPT_NONE, 
        OPT_PRINT_ALL, OPT_PRINT_SCREEN, OPT_SAVE_ALL, OPT_SAVE_STATE, 
        OPT_SAVE_TIMING, OPT_SAVE_CONFIG, OPT_SAVE_ANALOG, OPT_RESTORE, } opt = OPT_NONE;
    char *instrument = INSTRUMENT;
    int d, c;
    int local = 0;
    int clear = 0;
    int verbose = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'n':   /* --name */
            instrument = optarg;
            break;
        case 'l':   /* --local */
            local = 1;
            break;
        case 'v':   /* --verbose */
            verbose = 1;
            break;
        case 'c':   /* --clear */
            clear = 1;
            break;
        case 'p':   /* --print-screen */
            opt = OPT_PRINT_SCREEN; 
            break;
        case 'P':   /* --print-all */
            opt = OPT_PRINT_ALL; 
            break;
        case 's': 
            opt = OPT_SAVE_ALL; 
            break;
        case 'C': 
            opt = OPT_SAVE_CONFIG; 
            break;
        case 'A': 
            opt = OPT_SAVE_STATE; 
            break;
        case 't': 
            opt = OPT_SAVE_TIMING; 
            break;
        case 'a': 
            opt = OPT_SAVE_ANALOG; 
            break;
        case 'r': 
            opt = OPT_RESTORE; 
            break;
        }
    }

    if (opt == OPT_NONE && !clear && !local)
        usage();

    d = gpib_init(prog, instrument, verbose);

    if (clear) {
        gpib_ibclr(d);
        sleep(1);
        gpib_ibwrtf(d, "%s\r\n", HP1630_CMD_POWER_UP);/* doesn't affect date */
        hp1630_checksrq(d);
        hp1630_verify_model(d);
        /* set the errors _checksrq() will see */
        gpib_ibwrtf(d, "%s %d\r\n", HP1630_CMD_SET_SRQ_MASK, 
                HP1630_STAT_ERROR_LAST_CMD);
        hp1630_checksrq(d);
        hp1630_setdate(d);
    }

    /* Ops involving stdin/stdout are mutually exclusive.
     */
    switch (opt) {

    /* print */
    case OPT_PRINT_SCREEN:
        hp1630_printscreen(d, 0);
        break;
    case OPT_PRINT_ALL:
        hp1630_printscreen(d, 1);
        break;

    /* save/restore state */
    case OPT_SAVE_ALL:
        hp1630_savestate(d, HP1630_CMD_TRANSMIT_ALL);
        break;
    case OPT_SAVE_CONFIG:
        hp1630_savestate(d, HP1630_CMD_TRANSMIT_CONFIG);
        break;
    case OPT_SAVE_STATE:
        hp1630_savestate(d, HP1630_CMD_TRANSMIT_STATE);
        break;
    case OPT_SAVE_TIMING:
        hp1630_savestate(d, HP1630_CMD_TRANSMIT_TIMING);
        break;
    case OPT_SAVE_ANALOG: /* 1631 only */
        hp1630_savestate(d, HP1630_CMD_TRANSMIT_ANALOG);
        break;
    case OPT_RESTORE:
        hp1630_restorestate(d);
        break;
    case OPT_NONE:
        break;
    }

    if (local)
        gpib_ibloc(d);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
