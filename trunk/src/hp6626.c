/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2008-2009 Jim Garlick <garlick.jim@gmail.com>

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

#define INSTRUMENT "hp6626"

/* serial poll reg */
#define HP6626_SPOLL_FAU1   1
#define HP6626_SPOLL_FAU2   2
#define HP6626_SPOLL_FAU3   4
#define HP6626_SPOLL_FAU4   8
#define HP6626_SPOLL_RDY    16
#define HP6626_SPOLL_ERR    32
#define HP6626_SPOLL_RQS    64
#define HP6626_SPOLL_PON    128

/* status reg. bits (STS? - also ASTS? UNMASK? FAULT?) */
#define HP6626_STAT_CV      1    /* constant voltage mode */
#define HP6626_STAT_CCP     2    /* constant current (+) */
#define HP6626_STAT_CCM     4    /* constant current (-) */
#define HP6626_STAT_OV      8    /* over-voltage protection tripped */
#define HP6626_STAT_OT      16   /* over-temperature protection tripped */
#define HP6626_STAT_UNR     32   /* unregulated mode */
#define HP6626_STAT_OC      64   /* over-current protection tripped */
#define HP6626_STAT_CP      128  /* coupled parameter */

/* err reg values (ERR?) */
static strtab_t petab[] = {
    { 0,  "Success" },
    { 1,  "Invalid character" },
    { 2,  "Invalid number" },
    { 3,  "Invalid string" },
    { 4,  "Syntax error" },
    { 5,  "Number out of range" },
    { 6,  "Data requested without a query being sent" },
    { 7,  "Display length exceeded" },
    { 8,  "Buffer full" },
    { 9,  "EEPROM error" },
    { 10, "Hardware error" },
    { 11, "Hardware error on channel 1" },
    { 12, "Hardware error on channel 2" },
    { 13, "Hardware error on channel 3" },
    { 14, "Hardware error on channel 4" },
    { 15, "No model number" },
    { 16, "Calibration error" },
    { 17, "Uncalibrated" },
    { 18, "Calibration locked" },
    { 22, "Skip self test jumper is installed" },
    { 0, NULL }
};

/* selftest failures */
static strtab_t sttab[] = {
    { 0,  "Passed" },
    { 20, "Timer test failed" },
    { 21, "RAM test failed" },
    { 27, "ROM test failed" },
    { 29, "EEPROM checksum failed" },
    { 30, "CMODE store limit has been reached" },
    { 0, NULL }
};

static void _usage(void);
static int _interpret_status(gd_t gd, unsigned char status, char *msg);
static int _readiv(gd_t gd, double *ip, double *vp);

char *prog = "";

#define OPTIONS "a:clviSI:V:o:s:p:n:r:R:O:C:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"verbose",         no_argument,       0, 'v'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"get-idn",         no_argument,       0, 'i'},
    {"selftest",        no_argument,       0, 'S'},
    {"iset",            required_argument, 0, 'I'},
    {"vset",            required_argument, 0, 'V'},
    {"out-enable",      required_argument, 0, 'o'},
    {"samples",         required_argument, 0, 's'},
    {"period",          required_argument, 0, 'p'},
    {"out-chan",        required_argument, 0, 'n'},
    {"vrange",          required_argument, 0, 'r'},
    {"irange",          required_argument, 0, 'R'},
    {"ovset",           required_argument, 0, 'O'},
    {"ocp",             required_argument, 0, 'C'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int
main(int argc, char *argv[])
{
    gd_t gd;
    int i, rc, c, print_usage = 0;
    int exit_val = 0;
    char tmpstr[64];
    int samples = 0;
    double period = 0.0;
    int showtime = 0;
    int need_output = 0;
    char *output = NULL;

    gd = gpib_init_args(argc, argv, OPTIONS, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage)
        _usage();
    if (!gd)
        exit(1);

    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'I':
            case 'V':
            case 'o':
            case 'r':
            case 'R':
            case 'O':
            case 'C':
                need_output = 1;
                break;
            case 'n':
                output = optarg;
                break;
        }
    }
    if (need_output && !output) {
        fprintf(stderr, "%s: one more more options require --out-chan\n", prog);
        exit_val = 1;
        goto done;
    }
    if (!need_output && output) {
        fprintf(stderr, "%s: no options require --out-chan\n", prog);
        exit_val = 1;
        goto done;
    }

    optind = 0;
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'a': /* -a and -v handled in gpib_init_args */
            case 'v':
                break;
            case 'c': /* --clear */
                gpib_clr(gd, 1000000);
		for (i = 1; i <= 4; i++)
                    gpib_wrtf(gd, "OUT %d,0;OVRST %d;OCRST %d\n", i, i, i);
		gpib_wrtf(gd, "CLR\n");
                sleep(1);
                break;
            case 'l': /* --local */
                gpib_loc(gd); 
                break;
            case 'i': /* --get-idn */
                gpib_wrtstr(gd, "ID?\n");
                gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
                printf("%s\n", tmpstr);
                break;
            case 'S': /* --selftest */
                gpib_wrtstr(gd, "TEST?\n");
                gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
                if (sscanf(tmpstr, "%d", &rc) == 1)
                    printf("self-test: %s\n", findstr(sttab ,rc));
                else
                    printf("self-test: %s\n", tmpstr);
                break;
            case 'I': /* --iset */
                gpib_wrtf(gd, "ISET %s,%s\n", output, optarg);
                break;
            case 'V': /* --vset */
                gpib_wrtf(gd, "VSET %s,%s\n", output, optarg);
                break;
            case 'o': /* --out-enable */
                gpib_wrtf(gd, "OUT %s,%s\n", output, optarg);
                break;
            case 'r': /* --vrset */
                gpib_wrtf(gd, "VRSET %s,%s\n", output, optarg);
                break;
            case 'R': /* --irset */
                gpib_wrtf(gd, "IRSET %s,%s\n", output, optarg);
                break;
            case 'O': /* --ovset */
                gpib_wrtf(gd, "OVSET %s,%s\n", output, optarg);
                break;
            case 'C': /* --ocp */
                gpib_wrtf(gd, "OCP %s,%s\n", output, optarg);
                break;
            case 's': /* --samples */
                samples = strtoul(optarg, NULL, 10);
                if (samples > 1)
                    showtime = 1;
                break;
            case 'p': /* --period */
                if (freqstr(optarg, &period) < 0) {
                    fprintf(stderr, "%s: error parsing period argument\n",prog);
                    fprintf(stderr, "%s: use units of: %s\n", prog, PERIOD_UNITS);
                    exit_val = 1;
                    break;
                }
                period = 1.0/period;
                break;
        }
    }

    if (samples > 0) {
        double t0, t1, t2;
        double i[4], v[4];

        t0 = 0;
        while (samples-- > 0) {
            t1 = gettime();
            if (t0 == 0)
                t0 = t1;
            if (_readiv(gd, i, v) == -1) {
                exit_val = 1;
                goto done;  
            }
            t2 = gettime();

            if (showtime)
                printf("%-3.3lf\t", (t1-t0));
            printf("%lf\t%lf\t%lf\t%lf",   i[0], v[0], i[1], v[1]);
            printf("%lf\t%lf\t%lf\t%lf\n", i[2], v[2], i[3], v[3]);
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
"  -a,--address     set instrument address [%s]\n"
"  -c,--clear       initialize instrument to default values\n"
"  -l,--local       return instrument to local operation on exit\n"
"  -v,--verbose     show protocol on stderr\n"
"  -i,--get-idn     print instrument model\n"
"  -S,--selftest    run selftest\n"
"  -n,--out-chan    select output channel (1-4)\n"
"  -I,--iset        set current for selected channel (amps)\n"
"  -V,--vset        set voltage for selected channel (volts)\n"
"  -R,--irange      set current range for selected channel (amps)\n"
"  -r,--vrange      set voltage range for selected channel (volts)\n"
"  -o,--out-enable  enable/disable selected channel (0|1)\n"
"  -O,--ovset       set overvoltage threshold for selected channel (volts)\n"
"  -C,--ocp         set overcurrent protection for selected channel (0|1)\n"
"  -s,--samples     number of samples [default: 0]\n"
"  -p,--period      sample period [default: 0]\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    char tmpstr[64];    
    int e, err = 0;

    if ((status & HP6626_SPOLL_FAU1)) {
        fprintf(stderr, "%s: device fault on channel 1\n", prog);
        /* FIXME: check FAULT? and/or ASTS? but only if unmasked */
        err = 1;
    }
    if ((status & HP6626_SPOLL_FAU2)) {
        fprintf(stderr, "%s: device fault on channel 2\n", prog);
        /* FIXME: check FAULT? and/or ASTS? but only if unmasked */
        err = 1;
    }
    if ((status & HP6626_SPOLL_FAU3)) {
        fprintf(stderr, "%s: device fault on channel 3\n", prog);
        /* FIXME: check FAULT? and/or ASTS? but only if unmasked */
        err = 1;
    }
    if ((status & HP6626_SPOLL_FAU4)) {
        fprintf(stderr, "%s: device fault on channel 4\n", prog);
        /* FIXME: check FAULT? and/or ASTS? but only if unmasked */
        err = 1;
    }
    if ((status & HP6626_SPOLL_PON)) {
        /*fprintf(stderr, "%s: power-on detected\n", prog);*/
    }
    if ((status & HP6626_SPOLL_RDY)) {
    }
    if ((status & HP6626_SPOLL_ERR)) {
        gpib_wrtstr(gd, "ERR?\n");
        gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
        if (sscanf(tmpstr, "%d", &e) == 1)
            fprintf(stderr, "%s: prog error: %s\n", prog, findstr(petab, e));
        else
            fprintf(stderr, "%s: prog error: %s\n", prog, tmpstr);
        err = 1;
    }
    if ((status & HP6626_SPOLL_RQS)) {
        fprintf(stderr, "%s: device is requesting service\n", prog);
        err = 1; /* it shouldn't be unless we tell it to */
    }
    return err;
}

static int 
_readiv(gd_t gd, double *ip, double *vp)
{
    int i;

    for (i = 1; i <= 4; i++) {
        gpib_wrtf(gd, "VOUT? %d\n", i);
        if (gpib_rdf(gd, "%lf", &vp[i]) != 1) {
            fprintf(stderr, "%s: error reading VOUT? %d result\n", prog, i);
            return -1;
        }
        gpib_wrtf(gd, "IOUT? %d\n", i);
        if (gpib_rdf(gd, "%lf", &ip[i]) != 1) {
            fprintf(stderr, "%s: error reading IOUT? %d result\n", prog, i);
            return -1;
        }
    }
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
