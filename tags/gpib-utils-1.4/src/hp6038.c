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

#include "gpib.h"
#include "util.h"

#define INSTRUMENT "hp6038"

/* serial poll reg */
#define HP6038_SPOLL_FAU    1
#define HP6038_SPOLL_PON    2
#define HP6038_SPOLL_RDY    16
#define HP6038_SPOLL_ERR    32
#define HP6038_SPOLL_RQS    64

/* status reg. bits (STS? - also ASTS? UNMASK? FAULT?) */
#define HP6038_STAT_CV      1
#define HP6038_STAT_CC      2
#define HP6038_STAT_OR      4
#define HP6038_STAT_OV      8
#define HP6038_STAT_OT      16
#define HP6038_STAT_AC      32
#define HP6038_STAT_FOLD    64
#define HP6038_STAT_ERR     128
#define HP6038_STAT_RI      256

/* err reg values (ERR?) */
static strtab_t petab[] = {
    { 0, "Success" },
    { 1, "Unrecognized character" },
    { 2, "Improper number" },
    { 3, "Unrecognized string" },
    { 4, "Syntax error" },
    { 5, "Number out of range" },
    { 6, "Attempt to exceed soft limits" },
    { 7, "Improper soft limit" },
    { 8, "Data requested without a query being sent" },
    { 9, "Relay error" },
    { 0, NULL }
};

/* selftest failures */
static strtab_t sttab[] = {
    { 0,  "Passed" },
    { 4,  "External RAM test failed" },
    { 5,  "Internal RAM test failed" },
    { 6,  "External ROM test failed" },
    { 7,  "GPIB test failed" },
    { 8,  "GPIB address set to 31" },
    { 10, "Internal ROM test failed" },
    { 12, "ADC zero too high" },
    { 13, "Voltage DAC full scale low" },
    { 14, "Voltage DAC full scale high" },
    { 15, "Voltage DAC zero low" },
    { 16, "Voltage DAC zero high" },
    { 17, "Current DAC full scale low" },
    { 18, "Current DAC full scale high" },
    { 19, "Current DAC zero low" },
    { 20, "Current DAC zero high" },
    { 0, NULL }
};

static void _usage(void);
static int _interpret_status(gd_t gd, unsigned char status, char *msg);
static int _readiv(gd_t gd, double *ip, double *vp);

char *prog = "";

#define OPTIONS "a:clviSI:V:o:s:p:F:"
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
    {"out",             required_argument, 0, 'o'},
    {"foldback",        required_argument, 0, 'F'},
    {"samples",         required_argument, 0, 's'},
    {"period",          required_argument, 0, 'p'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int
main(int argc, char *argv[])
{
    gd_t gd;
    int rc, c, print_usage = 0;
    int exit_val = 0;
    char tmpstr[64];
    int samples = 0;
    double period = 0.0;
    int showtime = 0;

    gd = gpib_init_args(argc, argv, OPTIONS, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage)
        _usage();
    if (!gd)
        exit(1);

    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
        case 'a': /* -a and -v handled in gpib_init_args */
        case 'v':
            break;
        case 'c': /* --clear */
            gpib_clr(gd, 0); /* same as 'CLR' command */
            gpib_wrtf(gd, "OUT 0;RST\n");
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
            if (sscanf(tmpstr, "TEST %d", &rc) == 1)
                printf("self-test: %s\n", findstr(sttab ,rc));
            else
                printf("self-test: %s\n", tmpstr);
            break;
        case 'I': /* --iset */
            gpib_wrtf(gd, "ISET %s\n", optarg);
            break;
        case 'V': /* --vset */
            gpib_wrtf(gd, "VSET %s\n", optarg);
            break;
        case 'o': /* --out */
            gpib_wrtf(gd, "OUT %s\n", optarg);
            break;
        case 'F': /* --foldback */
            /* N.B. do all models accept OFF|CV|CC?  If not, cvt to 0|1|2 */
            gpib_wrtf(gd, "FOLD %s\n", optarg);
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
        }
    }

    if (samples > 0) {
        double t0, t1, t2;
        double i, v;

        t0 = 0;
        while (samples-- > 0) {
            t1 = gettime();
            if (t0 == 0)
                t0 = t1;
            if (_readiv(gd, &i, &v) == -1) {
                exit_val = 1;
                goto done;  
            }
            t2 = gettime();

            if (showtime)
                printf("%-3.3lf\t%lf\t%lf\n", (t1-t0), i, v);
            else
                printf("%lf\t%lf\n", i, v);
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
"  -a,--address            set instrument address [%s]\n"
"  -c,--clear              initialize instrument to default values\n"
"  -l,--local              return instrument to local operation on exit\n"
"  -v,--verbose            show protocol on stderr\n"
"  -i,--get-idn            print instrument model\n"
"  -S,--selftest           run selftest\n"
"  -I,--iset               set current\n"
"  -V,--vset               set voltage\n"
"  -o,--out                enable/disable output (0|1)\n"
"  -F,--foldback           set foldback (off|cv|cc)\n"
"  -s,--samples            number of samples [0]\n"
"  -p,--period             sample period\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    char tmpstr[64];    
    int e, err = 0;

    if (status & HP6038_SPOLL_FAU) {
        fprintf(stderr, "%s: device fault\n", prog);
        /* FIXME: check FAULT? and/or ASTS? but only if unmasked */
        err = 1;
    }
    if (status & HP6038_SPOLL_PON) {
        /*fprintf(stderr, "%s: power-on detected\n", prog);*/
    }
    if (status & HP6038_SPOLL_RDY) {
    }
    if (status & HP6038_SPOLL_ERR) {
        gpib_wrtstr(gd, "ERR?\n");
        gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
        if (sscanf(tmpstr, "%d", &e) == 1)
            fprintf(stderr, "%s: prog error: %s\n", prog, findstr(petab, e));
        else
            fprintf(stderr, "%s: prog error: %s\n", prog, tmpstr);
        err = 1;
    }
    if (status & HP6038_SPOLL_RQS) {
        fprintf(stderr, "%s: device is requesting service\n", prog);
        err = 1; /* it shouldn't be unless we tell it to */
    }
    return err;
}

static int 
_readiv(gd_t gd, double *ip, double *vp)
{
    gpib_wrtstr(gd, "VOUT?\n");
    if (gpib_rdf(gd, "%lf", vp) != 1) {
        fprintf(stderr, "%s: error reading VOUT? result\n", prog);
        return -1;
    }
    gpib_wrtstr(gd, "IOUT?\n");
    if (gpib_rdf(gd, "%lf", ip) != 1) {
        fprintf(stderr, "%s: error reading IOUT? result\n", prog);
        return -1;
    }
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
