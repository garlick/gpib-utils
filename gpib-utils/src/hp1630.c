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
#include "util.h"

static char *prog;

/* value of status byte 4 */
static errstr_t _sb4_errors[] = SB4_ERRORS;

#define INSTRUMENT "hp1630" /* the /etc/gpib.conf entry */
#define PATH_DATE  "/bin/date"

#define MAXCONFBUF (16*1024)
#define MAXPRINTBUF (128*1024)
#define MAXMODELBUF (16)

#define OPTIONS "n:clvpPsCAtar"
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
            fprintf(stderr, "%s: srq: problem with last command\n", prog);
            exit(1);
        }
    }
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
    fprintf(stderr, "%s: Print %s: %d bytes (%s format)\n", prog,
            allflag ? "all" : "screen", count,
            _hpgraphics(buf, count) ? "HP graphics" : "HP text");
    if (write_all(1, buf, count) < 0) {
        perror("write");
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

/* Set the date on the analyzer to match the UNIX date.
 */
static void
hp1630_setdate(int d)
{
    char datestr[64];
    int month;
	int i;

    _getdate(&month, datestr, 64); /* get date from UNIX */

	/* Position cursor on month, clear entry to reset month to Jan,
     * then hit 'next' enough times to advance to the desired month.
	 */
	gpib_ibwrtf(d, "%s;%s;%s;%s\r\n", HP1630_KEY_SYSTEM_MENU, HP1630_KEY_NEXT,
                    HP1630_KEY_CURSOR_DOWN, HP1630_KEY_CLEAR_ENTRY);
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

/* Return a string representation of learn string command,
 * or NULL if invalid.
 */
static char * 
_ls_cmd(uint8_t *ls)
{
    if (!strncmp((char *)&ls[0], "RC", 2))
        return "Config";
    else if (!strncmp((char *)&ls[0], "RS", 2))
        return "State";
    else if (!strncmp((char *)&ls[0], "RT", 2))
        return "Timing";
    else if (!strncmp((char *)&ls[0], "RA", 2))
        return "Analog";
    return NULL;
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

static uint16_t const hpcrc_table[256] = {
	0x0000, 0x8005, 0x800f, 0x000a, 0x801b, 0x001e, 0x0014, 0x8011,
	0x8033, 0x0036, 0x003c, 0x8039, 0x0028, 0x802d, 0x8027, 0x0022,
	0x8063, 0x0066, 0x006c, 0x8069, 0x0078, 0x807d, 0x8077, 0x0072,
	0x0050, 0x8055, 0x805f, 0x005a, 0x804b, 0x004e, 0x0044, 0x8041,
	0x80c3, 0x00c6, 0x00cc, 0x80c9, 0x00d8, 0x80dd, 0x80d7, 0x00d2,
	0x00f0, 0x80f5, 0x80ff, 0x00fa, 0x80eb, 0x00ee, 0x00e4, 0x80e1,
	0x00a0, 0x80a5, 0x80af, 0x00aa, 0x80bb, 0x00be, 0x00b4, 0x80b1,
	0x8093, 0x0096, 0x009c, 0x8099, 0x0088, 0x808d, 0x8087, 0x0082,
	0x8183, 0x0186, 0x018c, 0x8189, 0x0198, 0x819d, 0x8197, 0x0192,
	0x01b0, 0x81b5, 0x81bf, 0x01ba, 0x81ab, 0x01ae, 0x01a4, 0x81a1,
	0x01e0, 0x81e5, 0x81ef, 0x01ea, 0x81fb, 0x01fe, 0x01f4, 0x81f1,
	0x81d3, 0x01d6, 0x01dc, 0x81d9, 0x01c8, 0x81cd, 0x81c7, 0x01c2,
	0x0140, 0x8145, 0x814f, 0x014a, 0x815b, 0x015e, 0x0154, 0x8151,
	0x8173, 0x0176, 0x017c, 0x8179, 0x0168, 0x816d, 0x8167, 0x0162,
	0x8123, 0x0126, 0x012c, 0x8129, 0x0138, 0x813d, 0x8137, 0x0132,
	0x0110, 0x8115, 0x811f, 0x011a, 0x810b, 0x010e, 0x0104, 0x8101,
	0x8303, 0x0306, 0x030c, 0x8309, 0x0318, 0x831d, 0x8317, 0x0312,
	0x0330, 0x8335, 0x833f, 0x033a, 0x832b, 0x032e, 0x0324, 0x8321,
	0x0360, 0x8365, 0x836f, 0x036a, 0x837b, 0x037e, 0x0374, 0x8371,
	0x8353, 0x0356, 0x035c, 0x8359, 0x0348, 0x834d, 0x8347, 0x0342,
	0x03c0, 0x83c5, 0x83cf, 0x03ca, 0x83db, 0x03de, 0x03d4, 0x83d1,
	0x83f3, 0x03f6, 0x03fc, 0x83f9, 0x03e8, 0x83ed, 0x83e7, 0x03e2,
	0x83a3, 0x03a6, 0x03ac, 0x83a9, 0x03b8, 0x83bd, 0x83b7, 0x03b2,
	0x0390, 0x8395, 0x839f, 0x039a, 0x838b, 0x038e, 0x0384, 0x8381,
	0x0280, 0x8285, 0x828f, 0x028a, 0x829b, 0x029e, 0x0294, 0x8291,
	0x82b3, 0x02b6, 0x02bc, 0x82b9, 0x02a8, 0x82ad, 0x82a7, 0x02a2,
	0x82e3, 0x02e6, 0x02ec, 0x82e9, 0x02f8, 0x82fd, 0x82f7, 0x02f2,
	0x02d0, 0x82d5, 0x82df, 0x02da, 0x82cb, 0x02ce, 0x02c4, 0x82c1,
	0x8243, 0x0246, 0x024c, 0x8249, 0x0258, 0x825d, 0x8257, 0x0252,
	0x0270, 0x8275, 0x827f, 0x027a, 0x826b, 0x026e, 0x0264, 0x8261,
	0x0220, 0x8225, 0x822f, 0x022a, 0x823b, 0x023e, 0x0234, 0x8231,
	0x8213, 0x0216, 0x021c, 0x8219, 0x0208, 0x820d, 0x8207, 0x0202,
};

static uint16_t 
hpcrc(uint8_t const *buffer, int len)
{
    uint16_t crc = 0;

    while (len--)
        crc = (crc & 0xff00) ^ hpcrc_table[(crc ^ *buffer++) & 0xff];

    return crc;
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
    if (_ls_crc(ls) != hpcrc(ls+4, len-2)) {
        fprintf(stderr, "%s: learn string CRC does not match\n", prog);
        exit(1);
    }

    fprintf(stderr, "%s: %s: %-4.4d+4 bytes\n", prog, cmd, len);

    return len + 4;
}

static void
hp1630_restore(int d)
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
        gpib_ibwrt(d, buf + i, lslen);
        hp1630_checksrq(d);

        /* get status byte 4 and check for error */
        gpib_ibwrtf(d, "%s 4\r\n", HP1630_CMD_STATUS_BYTE);
        hp1630_checksrq(d);
        count = gpib_ibrd(d, &status, 1);
        hp1630_checksrq(d);
        if (count != 1) {
            fprintf(stderr, "%s: error reading status byte 4\n", prog);
            exit(1);
        }
        if (status != 0)
            fprintf(stderr, "%s: %s\n", prog, finderr(_sb4_errors, status) );

        i += lslen;
    }
}

/* NOTE: if analyzer is not configured with any state lines (e.g. analog only
 * on a 1631), save all and save state will return 'unrecognized command'.
 */
static void
hp1630_save(int d, char *cmd)
{
    uint8_t buf[MAXCONFBUF];
    int count, len;
    uint8_t status;
    int i;

    gpib_ibwrtf(d, "%s\r\n", cmd);
    hp1630_checksrq(d);
    len = gpib_ibrd(d, buf, sizeof(buf));
    hp1630_checksrq(d);

    if (write_all(1, buf, len) < 0) {
        perror("write");
        exit(1);
    }

    /* get status byte 4 and check for error */
    gpib_ibwrtf(d, "%s 4\r\n", HP1630_CMD_STATUS_BYTE);
    hp1630_checksrq(d);
    count = gpib_ibrd(d, &status, 1);
    hp1630_checksrq(d);
    if (count != 1) {
        fprintf(stderr, "%s: error reading status byte 4\n", prog);
        exit(1);
    }
    if (status != 0)
        fprintf(stderr, "%s: %s\n", prog, finderr(_sb4_errors, status) );

    /* parse what was read */
    i = 0;
    while (i < len)
        i += _ls_parse(buf + i, len - i);
}


int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int d, c;
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
        }
    }

    if (!clear && !local && !print_screen && !print_all && !save 
            && !save_all && !restore)
        usage();
    if (save + save_all + restore + print_screen + print_all > 1)
        usage();

    d = gpib_init(prog, instrument, verbose);

    if (clear) {
        gpib_ibclr(d);
        sleep(1);
        gpib_ibwrtf(d, "%s\r\n", HP1630_CMD_POWER_UP);
        hp1630_checksrq(d);
        hp1630_verify_model(d);
        /* set the errors _checksrq() will see */
        gpib_ibwrtf(d, "%s %d\r\n", HP1630_CMD_SET_SRQ_MASK, 
                HP1630_STAT_ERROR_LAST_CMD);
        hp1630_checksrq(d);
        hp1630_setdate(d);
    }

    if (print_screen) {
        hp1630_printscreen(d, 0);
    } else if (print_all) {
        hp1630_printscreen(d, 1);
    } else if (restore) {
        hp1630_restore(d);
    } else if (save_all) {
        hp1630_save(d, HP1630_CMD_TRANSMIT_ALL);
    } else if (save) {
        /* Learn strings can be concatenated and restored as a unit.
         */
        if (save_config)
            hp1630_save(d, HP1630_CMD_TRANSMIT_CONFIG);
        if (save_state)
            hp1630_save(d, HP1630_CMD_TRANSMIT_STATE);
        if (save_timing)
            hp1630_save(d, HP1630_CMD_TRANSMIT_TIMING);
        if (save_analog)
            hp1630_save(d, HP1630_CMD_TRANSMIT_ANALOG);
    }

    if (local)
        gpib_ibloc(d);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
