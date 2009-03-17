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

/* Agilent 34401A DMM */
/* Agilent 34410A and 34411A DMM  */

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

#include "util.h"
#include "gpib.h"

#define INSTRUMENT "a34401"

/* Status byte values
 */
#define A34401_STAT_RQS  64      /* device is requesting service */
#define A34401_STAT_SER  32      /* bits are set in std. event register */
#define A34401_STAT_MAV  16      /* message(s) available in the output buffer */
#define A34401_STAT_QDR  8       /* bits are set in quest. data register */

/* Standard event register values.
 */
#define A34401_SER_PON   128     /* power on */
#define A34401_SER_CME   32      /* command error */
#define A34401_SER_EXE   16      /* execution error */
#define A34401_SER_DDE   8       /* device dependent error */
#define A34401_SER_QYE   4       /* query error */
#define A34401_SER_OPC   1       /* operation complete */

/* Questionable data register.
 */
#define A34401_QDR_LFH   4096    /* limit fail HI */
#define A34401_QDR_LFL   2048    /* limit fail LO */
#define A34401_QDR_ROL   512     /* ohms overload */
#define A34401_QDR_AOL   2       /* current overload */
#define A34401_QDR_VOL   1       /* voltage overload */

#define SETUP_STR_SIZE  3000     /* 2617 actually */

static int _interpret_status(gd_t gd, unsigned char status, char *msg);

char *prog = "";
const char *options = OPTIONS_COMMON "lcisoRrSf:g:G:t:N:p:";

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
    {"function",        required_argument, 0, 'f'},
    {"range",           required_argument, 0, 'g'},
    {"resolution",      required_argument, 0, 'G'},
    {"trigger",         required_argument, 0, 't'},
    {"samples",         required_argument, 0, 'N'},
    {"period",          required_argument, 0, 'p'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static opt_desc_t optdesc[] = {
    OPTIONS_COMMON_DESC,
    {"l", "local",       "return instrument to front panel control" },
    {"c", "clear",       "initialize instrument to default values" },
    {"R", "reset",       "reset instrument to std. settings" },
    {"S", "selftest",    "execute instrument self test" },
    {"i", "get-idn",     "display instrument idn string" },
    {"o", "get-opt",     "display instrument installed options" },
    {"s", "save-config", "save instrument config on stdout" },
    {"r", "restore",     "restore instrument config from stdin" },
    {"f", "function",   "select function" },
    {"g", "range",      "select range: (range|MIN|MAX|DEF)" },
    {"G", "resolution", "select resolution: (resolution|MIN|MAX|DEF)" },
    {"t", "trigger",    "select trigger mode" },
    {"N", "samples",    "number of samples [0]" },
    {"p", "period",     "sample period" },
    {0, 0},
};

int
main(int argc, char *argv[])
{
    int samples = 0, showtime = 0, exit_val = 0;
    double period = 0.0;
    int need_fun = 0, have_fun = 0, print_usage = 0;
    char range[16] = "DEF", resolution[16] = "DEF";
    gd_t gd;
    int c;

    gd = gpib_init_args(argc, argv, options, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage)
        usage(optdesc);
    if (!gd)
        exit(1);

    /* preparse args for range and resolution */
    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            case 'g': /* --range */
                snprintf(range, sizeof(range), "%s", optarg);
                need_fun++;
                break;
            case 'G': /* --resolution */
                snprintf(resolution, sizeof(resolution), "%s", optarg);
                need_fun++;
                break;
            case 'f': /* --function */
                have_fun++;
                break;
        }
    }
    if (need_fun && !have_fun) {
        fprintf(stderr, "%s: range/resolution require function\n", prog);
        exit_val = 1;
        goto done;
    }

    optind = 0;
    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            OPTIONS_COMMON_SWITCH
            case 'g': /* -g, -G handled above */
            case 'G':
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
            case 'f': /* --function */
                if (!strcmp(optarg, "dcv"))
                    gpib_wrtf(gd, "CONF:VOLT:DC %s,%s", range, resolution);
                else if (!strcmp(optarg, "acv"))
                    gpib_wrtf(gd, "CONF:VOLT:AC %s,%s", range, resolution);
                else if (!strcmp(optarg, "dci"))
                    gpib_wrtf(gd, "CONF:CURR:DC %s,%s", range, resolution);
                else if (!strcmp(optarg, "aci"))
                    gpib_wrtf(gd, "CONF:CURR:AC %s,%s", range, resolution);
                else if (!strcmp(optarg, "ohm") || !strcmp(optarg, "ohm2"))
                    gpib_wrtf(gd, "CONF:RES %s,%s", range, resolution);
                else if (!strcmp(optarg, "ohmf") || !strcmp(optarg, "ohm4"))
                    gpib_wrtf(gd, "CONF:FRES %s,%s", range, resolution);
                else if (!strcmp(optarg, "freq"))
                    gpib_wrtf(gd, "CONF:FREQ %s,%s", range, resolution);
                else if (!strcmp(optarg, "period"))
                    gpib_wrtf(gd, "CONF:PER %s,%s", range, resolution);
                else {
                    fprintf(stderr, "%s: error parsing function arg\n", prog);
                    exit_val = 1;
                }
                break;
            case 't': /* --trigger */
                if (!strcmp(optarg, "imm"))
                    gpib_wrtf(gd, "%s", "TRIG:SOUR IMM");
                else if (!strcmp(optarg, "ext"))
                    gpib_wrtf(gd, "%s", "TRIG:SOUR EXT");
                else if (!strcmp(optarg, "bus"))
                    gpib_wrtf(gd, "%s", "TRIG:SOUR BUS");
                else {
                    fprintf(stderr, "%s: error parsing trigger arg\n", prog);
                    exit_val = 1;
                }
                break;
            case 'N': /* --samples */
                samples = strtoul(optarg, NULL, 10);
                if (samples > 1)
                    showtime = 1;
                break;
            case 'p': /* --period */
                if (freqstr(optarg, &period) < 0) {
                    fprintf(stderr, "%s: error parsing period argument\n", prog);
                    fprintf(stderr, "%s: use units of: %s\n", prog, PERIOD_UNITS);
                    exit_val = 1; 
                    break;
                }
                period = 1.0/period;
                break;
        }
    }
    if (exit_val != 0)
        goto done;

    if (samples > 0) {
        char buf[64];
        double t0, t1, t2;

        t0 = 0;
        while (samples-- > 0) {
            t1 = gettime();
            if (t0 == 0)
                t0 = t1;

            gpib_wrtf(gd, "%s", "READ?");
            gpib_rdstr(gd, buf, sizeof(buf));
            t2 = gettime();

            if (showtime)
                printf("%-3.3lf\t%s\n", (t1-t0), buf);
            else
                printf("%s\n", buf);
            if (samples > 0 && period > 0)
                sleep_sec(period - (t2-t1));
        }
    }

done:
    gpib_fini(gd);
    exit(exit_val);
}

#if 0
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
#endif

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

    if (status & A34401_STAT_RQS) {
    }
    if (status & A34401_STAT_SER) {
    }
    if (status & A34401_STAT_MAV) {
    }
    if (status & A34401_STAT_QDR) {
    }

    return err;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
