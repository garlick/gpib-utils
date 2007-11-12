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

#include "gpib.h"
#include "hp1630.h"
#include "util.h"
#include "hpcrc.h"

char *prog;

/* value of status byte 4 */
static strtab_t _sb4_errors[] = SB4_ERRORS;

#define INSTRUMENT "hp1630" /* the /etc/gpib.conf entry */
#define PATH_DATE  "/bin/date"

#define MAXCONFBUF (16*1024)
#define MAXPRINTBUF (128*1024)
#define MAXMODELBUF (16)

#define OPTIONS "n:clvpPsCAtard"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument, 0, 'c'},
    {"local",           no_argument, 0, 'l'},
    {"verbose",         no_argument, 0, 'v'},
    {"print-screen",    no_argument, 0, 'p'},
    {"print-all",       no_argument, 0, 'P'},
    {"save-all",        no_argument, 0, 's'},
    {"save-config",     no_argument, 0, 'C'},
    {"save-state",      no_argument, 0, 'A'},
    {"save-timing",     no_argument, 0, 't'},
    {"save-analog",     no_argument, 0, 'a'},
    {"restore",         no_argument, 0, 'r'},
    {"date",            no_argument, 0, 'd'},
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
"  -d,--date          set date\n"
           , prog, INSTRUMENT);
    exit(1);
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
hp1630_printscreen(gd_t gd, int allflag)
{
    uint8_t buf[MAXPRINTBUF];
    int count;

    if (allflag)
        gpib_wrtf(gd, "%s\r\n", HP1630_KEY_PRINT_ALL);
    else
        gpib_wrtf(gd, "%s\r\n", HP1630_KEY_PRINT);
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

/* Set the date on the analyzer to match the UNIX date.
 */
static void
hp1630_setdate(gd_t gd)
{
    char datestr[64];
    int month;
	int i;

    _getdate(&month, datestr, 64); /* get date from UNIX */

	/* Position cursor on month, clear entry to reset month to Jan,
     * then hit 'next' enough times to advance to the desired month.
	 */
	gpib_wrtf(gd, "%s;%s;%s;%s\r\n", HP1630_KEY_SYSTEM_MENU, HP1630_KEY_NEXT,
                    HP1630_KEY_CURSOR_DOWN, HP1630_KEY_CLEAR_ENTRY);
	for (i = 1; i < month; i++)       /* 1 = jan, 2 = feb, etc. */
		gpib_wrtf(gd, "%s\r\n", HP1630_KEY_NEXT);
	/* position over day and spew forth the remaining digits */
    gpib_wrtf(gd, "%s;\"%s\"\r\n", HP1630_KEY_CURSOR_RIGHT, datestr);
}

static void
hp1630_verify_model(gd_t gd)
{
    char *supported[] = { "HP1630A", "HP1630D", "HP1630G",
                          "HP1631A", "HP1631D", NULL };
    char buf[MAXMODELBUF];
    int found = 0;
    int i;

	gpib_wrtf(gd, "%s\r\n", HP1630_CMD_SEND_IDENT);
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
hp1630_restore(gd_t gd)
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

        /* write the learn string command */
        gpib_wrt(gd, buf + i, lslen);
        /* XXX The following string is written after each learn string
         * to work around a possible HP1630D firmware bug identified by
         * Adam Goldman where the analyzer seems to be inappropriately 
         * waiting for more data in some cases.
         */
        gpib_wrtf(gd,"\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");

        /* get status byte 4 and check for error */
        gpib_wrtf(gd, "%s 4\r\n", HP1630_CMD_STATUS_BYTE);
        count = gpib_rd(gd, &status, 1);
        if (count != 1) {
            fprintf(stderr, "%s: error reading status byte 4\n", prog);
            exit(1);
        }
        if (status != 0)
            fprintf(stderr, "%s: %s\n", prog, findstr(_sb4_errors, status) );

        i += lslen;
    }
}

/* NOTE: if analyzer is not configured with any state lines (e.g. analog only
 * on a 1631), save all and save state will return 'unrecognized command'.
 */
static void
hp1630_save(gd_t gd, char *cmd)
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
    gpib_wrtf(gd, "%s 4\r\n", HP1630_CMD_STATUS_BYTE);
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


int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    gd_t gd;
    int c;
    int local = 0;
    int clear = 0;
    int verbose = 0;
    int print_all = 0;
    int print_screen = 0;
    int save_all = 0;
    int save = 0;
    int save_state = 0;
    int save_timing = 0;
    int save_config = 0;
    int save_analog = 0;
    int restore = 0;
    int date = 0;

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
            print_screen = 1;
            break;
        case 'P':   /* --print-all */
            print_all = 1;
            break;
        case 's':   /* --save-all */
            save_all = 1;
            break;
        case 'C':   /* --save-config */
            save = 1;
            save_config = 1;
            break;
        case 'A':   /* --save-state */
            save = 1;
            save_state = 1;
            break;
        case 't':   /* --save-timing */
            save = 1;
            save_timing = 1;
            break;
        case 'a':   /* --save-analog (1631 only) */
            save = 1;
            save_analog = 1;
            break;
        case 'r':   /* --restore */
            restore = 1;
            break;
        case 'd':   /* --date */
            date = 1;
            break;
        }
    }

    if (!clear && !local && !print_screen && !print_all && !save 
            && !save_all && !restore)
        usage();
    if (save + save_all + restore + print_screen + print_all > 1)
        usage();

    gd = gpib_init(instrument, _interpret_status, 0);
    if (!gd) {
        fprintf(stderr, "%s: couldn't find device %s in /etc/gpib.conf\n",                 prog, instrument);
        exit(1);
    }
    gpib_set_verbose(gd, verbose);

    if (clear) {
        gpib_clr(gd, 1000000);
        gpib_wrtf(gd, "%s\r\n", HP1630_CMD_POWER_UP);
        hp1630_verify_model(gd);
        /* set the errors _checksrq() will see */
        gpib_wrtf(gd, "%s %d\r\n", HP1630_CMD_SET_SRQ_MASK, 
                HP1630_STAT_ERROR_LAST_CMD);
    }
    if (clear || date)
        hp1630_setdate(gd);

    if (print_screen) {
        hp1630_printscreen(gd, 0);
    } else if (print_all) {
        hp1630_printscreen(gd, 1);
    } else if (restore) {
        hp1630_restore(gd);
    } else if (save_all) {
        hp1630_save(gd, HP1630_CMD_TRANSMIT_ALL);
    } else if (save) {
        /* Learn strings can be concatenated and restored as a unit.
         */
        if (save_config)
            hp1630_save(gd, HP1630_CMD_TRANSMIT_CONFIG);
        if (save_state)
            hp1630_save(gd, HP1630_CMD_TRANSMIT_STATE);
        if (save_timing)
            hp1630_save(gd, HP1630_CMD_TRANSMIT_TIMING);
        if (save_analog)
            hp1630_save(gd, HP1630_CMD_TRANSMIT_ANALOG);
    }

    if (local)
        gpib_loc(gd);

    gpib_fini(gd);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
