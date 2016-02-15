/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2007-2009 Jim Garlick <garlick.jim@gmail.com>

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

/* Agilent 6000 series of oscilloscopes (DSO and MSO models) */

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#if HAVE_GETOPT_LONG
#include <getopt.h>
#endif
#include <assert.h>
#include <sys/time.h>
#include <math.h>
#include <stdint.h>

#include "libutil/util.h"
#include "gpib.h"

#define PATH_DATE "/bin/date"

#define INSTRUMENT "a6032"

#define SETUP_STR_SIZE  3000
#define IMAGE_SIZE      6000000

/* Status byte values
 */
#define A6032_STAT_OPER   128    /* an enabled condition in OPER has occurred */
#define A6032_STAT_RQS    32    /* device is requesting service */
#define A6032_STAT_ESB    16    /* an enabled condition in ESR has occurred */
#define A6032_STAT_MAV    8    /* message(s) available in the output queue */
#define A6032_STAT_MSG    4    /* advisory has been displayed on the scope */
#define A6032_STAT_USR    2    /* an enabled user event has occurred */
#define A6032_STAT_TRG    1    /* trigger has occurred */

/* Event status register values
 */
#define A6032_ESR_PON    128    /* power on */
#define A6032_ESR_URQ    64    /* user request */
#define A6032_ESR_CME    32    /* command error */
#define A6032_ESR_EXE    16    /* execution error */
#define A6032_ESR_DDE    8    /* device dependent error */
#define A6032_ESR_QYE    4    /* query error */
#define A6032_ESR_RQL    2    /* request control */
#define A6032_ESR_OPC    1    /* operation complete */

static int _print_screen(gd_t gd, char *fmt, char *palette);
static int _setdate(gd_t gd);
static int _interpret_status(gd_t gd, unsigned char status, char *msg);

char *prog = "";
static char *options = OPTIONS_COMMON "lcisoRrSpf:P:d";


#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    OPTIONS_COMMON_LONG,
    {"local",           no_argument,       0, 'l'},
    {"clear",           no_argument,       0, 'c'},
    {"get-idn",         no_argument,       0, 'i'},
    {"save-config",     no_argument,       0, 's'},
    {"get-opt",         no_argument,       0, 'o'},
    {"reset",           no_argument,       0, 'R'},
    {"restore",         no_argument,       0, 'r'},
    {"selftest",        no_argument,       0, 'S'},
    {"print-screen",    no_argument,       0, 'p'},
    {"print-format",    required_argument, 0, 'f'},
    {"print-palette",   required_argument, 0, 'P'},
    {"set-date",        no_argument,       0, 'd'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static opt_desc_t optdesc[] = {
    OPTIONS_COMMON_DESC,
    {"l", "local",          "return instrument to front panel control" },
    {"c", "clear",          "initialize instrument to default values" },
    {"R", "reset",          "reset instrument to std. settings" },
    {"S", "selftest",       "execute instrument self test" },
    {"i", "get-idn",        "display instrument idn string" },
    {"o", "get-opt",        "display instrument installed options" },
    {"s", "save-config",    "save instrument config on stdout" },
    {"r", "restore",        "restore instrument config from stdin" },
    { "p", "print-screen",  "send screen dump to stdout" },
    { "f", "print-format",  "select print format (bmp|bmp8bit|png) [png]" },
    { "P", "print-palette", "select print palette (gray|col) [col]" },
    { "d", "set-date",      "set date and time to match UNIX" },
    { 0, 0 },
};

int
main(int argc, char *argv[])
{
    gd_t gd;
    int c, exit_val = 0, print_usage = 0;
    char *print_format = "png";
    char *print_palette = "col";

    gd = gpib_init_args(argc, argv, options, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage)
        usage(optdesc);
    if (!gd)
        exit(1);

    /* preparse print modifiers */
    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            case 'f': /* --print-format */
                if (       strncasecmp(optarg, "bmp", 3)
                        && strncasecmp(optarg, "png", 3)
                        && strncasecmp(optarg, "bmp8bit", 7)) {
                    fprintf(stderr, "%s: error parsing print-format\n", prog);
                    exit_val = 1;
                    break;
                }
                print_format = optarg;
                break;
            case 'P': /* --print-palette */
                if (       strncasecmp(optarg, "col", 3) 
                        && strncasecmp(optarg, "gray", 4)) {
                    fprintf(stderr, "%s: error parsing print-palette\n", prog);
                    exit_val = 1;
                    break;
                }
                print_palette = optarg;
                break;
        }
    }
    if (exit_val == 1)
        goto done;

    optind = 0;
    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            OPTIONS_COMMON_SWITCH
            case 'f': /* -f -P handled above */
            case 'P':
                break;
            case 'l': /* --local */
		gpib_loc(gd);
		break;
            case 'c': /* --clear */
                gpib_488_2_cls(gd);
                break;
            case 'R': /* --reset */
                gpib_488_2_rst(gd, 5);
                break;
            case 'S': /* --selftest */
                gpib_488_2_tst(gd, NULL);
                break;
            case 'i': /* --get-idn */
                gpib_488_2_idn(gd);
                break;
            case 'o': /* --get-opt */
                gpib_488_2_opt(gd);
                break;
            case 's': /* --save-config */
                gpib_488_2_lrn(gd);
                break;
            case 'r': /* --restore */
                gpib_488_2_restore(gd);
                break;
            case 'p': /* --print-screen */
                if (_print_screen(gd, print_format, print_palette) == -1)
                    exit_val = 1;
                break;
            case 'd': /* --set-date */
                if (_setdate(gd) == -1)
                    exit_val = 1;
                break;
        }
    }

done:
    gpib_fini(gd);
    exit(exit_val);
}


static void
_dump_error_queue(gd_t gd)
{
    char tmpstr[128];
    char *endptr;
    int err;

    do {
        gpib_wrtf(gd, ":SYSTEM:ERROR?");
        gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
        err = strtoul(tmpstr, &endptr, 10);
        if (err)
            fprintf(stderr, "%s: system error: %s\n", prog, endptr + 1);
    } while (err);
}

static int
_check_esr(gd_t gd)
{
    int esr;
    int err = 0;

    gpib_wrtf(gd, "*ESR?");
    if (gpib_rdf(gd, "%d", &esr) != 1) {
        fprintf(stderr, "%s: error reading event status register\n", prog);
        return 0;
    }
    if (esr & A6032_ESR_PON) {
        fprintf(stderr, "%s: power on\n", prog);
    }
    if (esr & A6032_ESR_URQ) {
        fprintf(stderr, "%s: user request\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_CME) {
        fprintf(stderr, "%s: command error\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_EXE) {
        fprintf(stderr, "%s: execution error\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_DDE) {
        fprintf(stderr, "%s: device dependent error\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_QYE) {
        fprintf(stderr, "%s: query error\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_RQL) {
        fprintf(stderr, "%s: request control\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_OPC) {
        fprintf(stderr, "%s: operation complete \n", prog);
    }
    if (err) 
        _dump_error_queue(gd);
    return err;
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

    if (status & A6032_STAT_OPER) {
        /* check with :OPEREGISTER:CONDITION? */
    }
    if (status & A6032_STAT_RQS) {
        fprintf(stderr, "%s: device is requesting service\n", prog);
        /* FIXME: how to clear? */
        /*err = 1;*/
    }
    if (status & A6032_STAT_ESB) {
        err = _check_esr(gd);
    }
    if (status & A6032_STAT_MAV) {
        fprintf(stderr, "%s: check output queue\n", prog);
    }
    if (status & A6032_STAT_MSG) {
        fprintf(stderr, "%s: advisory message has been displayed\n", prog);
        /* FIXME: how to clear? */
    }
    if (status & A6032_STAT_USR) {
        fprintf(stderr, "%s: user event has occurred\n", prog);
    }
    if (status & A6032_STAT_TRG) {
        /*fprintf(stderr, "%s: trigger has occurred\n", prog);*/
    }
    /*gpib_wrtf(gd, "*CLS");*/
    return err;
}

static int
_getunixdate(char *datestr, int len)
{
    FILE *pipe;
    char cmd[80];

    if (snprintf(cmd, sizeof(cmd), "%s +'%%Y,%%m,%%d-%%H,%%M,%%S'", 
                 PATH_DATE) < 0) {
        fprintf(stderr, "%s: out of memory", prog);
        return -1;
    }
    if (!(pipe = popen(cmd, "r"))) {
        perror("popen");
        return -1;
    }
    if (fscanf(pipe, "%s", datestr) != 2)
        return -1;
    assert(strlen(datestr) < len);
    if (pclose(pipe) < 0) {
        perror("pclose");
        return -1;
    }
    return 0;
}

/* Set the date on the analyzer to match the UNIX date.
 */
static int
_setdate(gd_t gd)
{
    char datestr[64];
    char *timestr;

    _getunixdate(datestr, 64);
    timestr = strchr(datestr, '-');
    if (!timestr) {
        fprintf(stderr, "%s: error parsing UNIX date string\n", prog);
        return -1;
    }
    *timestr++ = '\0';

    gpib_wrtf(gd, ":SYSTEM:DATE %s", datestr);
    gpib_wrtf(gd, ":SYSTEM:TIME %s", timestr);
    return 0;
}

/* Print screen to stdout.
 */
static int
_print_screen(gd_t gd, char *fmt, char *palette)
{
    unsigned char *buf = xmalloc(IMAGE_SIZE);
    int len;
    char qry[64];

    snprintf(qry, sizeof(qry), ":DISPLAY:DATA? %s,scr,%s", fmt, palette);
    len = gpib_qry(gd, qry, buf, IMAGE_SIZE);

    if (gpib_decode_488_2_data(buf, &len, GPIB_DECODE_DLAB) == -1) {
        fprintf(stderr, "%s: failed to decode 488.2 data\n", prog);
        goto fail;
    }
    if (write_all(1, buf, len) < 0) {
        perror("write");
        goto fail;
    }
    fprintf(stderr, "%s: print screen: %d bytes (format=%s, palette=%s)\n", 
            prog, len, fmt, palette);
    free(buf);
    return 0;
fail:
    free(buf);
    return -1;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
