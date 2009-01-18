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

#include "gpib.h"
#include "util.h"

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

static void _usage(void);
static int _interpret_status(gd_t gd, unsigned char status, char *msg);
static int _get_idn(gd_t gd);
static int _restore_setup(gd_t gd);
static int _save_setup(gd_t gd);
static int _selftest(gd_t gd);

char *prog = "";

#define OPTIONS "a:clviSr:R:f:t:s:p:zZ"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"get-idn",         no_argument,       0, 'i'},
    {"selftest",        no_argument,       0, 'S'},
    {"function",        required_argument, 0, 'f'},
    {"range",           required_argument, 0, 'r'},
    {"resolution",      required_argument, 0, 'R'},
    {"trigger",         required_argument, 0, 't'},
    {"samples",         required_argument, 0, 's'},
    {"period",          required_argument, 0, 'p'},
    {"save-setup",      no_argument,       0, 'z'},
    {"restore-setup",   no_argument,       0, 'Z'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int
main(int argc, char *argv[])
{
    int samples = 0, showtime = 0, exit_val = 0;
    double period = 0.0;
    int need_fun = 0, have_fun = 0, print_usage = 0;
    char range[16] = "DEF", resolution[16] = "DEF";
    gd_t gd;
    int c;

    gd = gpib_init_args(argc, argv, OPTIONS, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage)
        _usage();
    if (!gd)
        exit(1);

    /* preparse args for range and resolution */
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'r': /* --range */
                snprintf(range, sizeof(range), "%s", optarg);
                need_fun++;
                break;
            case 'R': /* --resolution */
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
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'a': /* -a and -v handled in gpib_init_args() */
            case 'v': 
            case 'r': /* -r and -R preparsed above */
            case 'R':
                break;
            case 'c': /* --clear */
                gpib_clr(gd, 0);
                gpib_wrtf(gd, "*CLS");
                gpib_wrtf(gd, "*RST");
                sleep(1);
                break;
            case 'l': /* --local */
                gpib_loc(gd); 
                break;
            case 'i': /* --get-idn */
                if (_get_idn(gd) == -1)
                    exit_val = 1;
                break;
            case 'S': /* --selftest */
                if (_selftest(gd) == -1)
                    exit_val = 1;
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
            case 's': /* --samples */
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
            case 'z': /* --save-setup */
                _save_setup(gd);
                break;
            case 'Z': /* --restore-setup */
                _restore_setup(gd);
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

static void 
_usage(void)
{
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -a,--address               set instrument address [%s]\n"
"  -c,--clear                 initialize instrument to default values\n"
"  -l,--local                 return instrument to local op on exit\n"
"  -v,--verbose               show protocol on stderr\n"
"  -i,--get-idn               return instrument idn string\n"
"  -S,--selftest              perform instrument self-test\n"
"  -f,--function dcv|acv|dci|aci|ohm2|ohm4|freq|period\n"
"                             select function [dcv]\n"
"  -r,--range                 set range: range|MIN|MAX|DEF\n"
"  -R,--resolution            set resolution: resolution|MIN|MAX|DEF\n"
"  -t,--trigger imm|ext|bus   select trigger mode\n"
"  -s,--samples               number of samples [0]\n"
"  -p,--period                sample period\n"
"  -z,--save-setup            save setup to stdout\n"
"  -Z,--restore-setup         restore setup from stdin\n"
           , prog, addr ? addr : "no default");
    exit(1);
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

/* Save setup to stdout.
 */
static int
_save_setup(gd_t gd)
{
    unsigned char *buf = xmalloc(SETUP_STR_SIZE);
    int len;

    len = gpib_qry(gd, "*LRN?", buf, SETUP_STR_SIZE);
    if (write_all(1, buf, len) < 0) {
        perror("write");
        return -1;
    }
    fprintf(stderr, "%s: save setup: %d bytes\n", prog, len);
    free(buf);
    return 0;
}

/* Restore setup from stdin.
 */
static int
_restore_setup(gd_t gd)
{
    unsigned char buf[SETUP_STR_SIZE];
    int len;

    len = read_all(0, buf, sizeof(buf));
    if (len < 0) {
        perror("read");
        return -1;
    }
    gpib_wrt(gd, buf, len);
    fprintf(stderr, "%s: restore setup: %d bytes\n", prog, len);
    return 0;
}

/* Print instrument idn string.
 */
static int
_get_idn(gd_t gd)
{
    char tmpstr[64];

    gpib_qry(gd, "*IDN?", tmpstr, sizeof(tmpstr) - 1);
    fprintf(stderr, "%s: %s", prog, tmpstr);
    return 0;
}

static int
_selftest(gd_t gd)
{
    char buf[64];

    gpib_qry(gd, "*TST?", buf, sizeof(buf));
    switch (strtoul(buf, NULL, 10)) {
        case 0:
            printf("self-test completed successfully\n");
            return 0;
        case 1:
            printf("self-test failed\n");
            return -1;
        default:
            printf("self-test returned unexpected response\n");
            return -1;
    }
    /*NOTREACHED*/
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
