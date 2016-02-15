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

#include "libutil/util.h"
#include "gpib.h"

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

static int _interpret_status(gd_t gd, unsigned char status, char *msg);
static int _readiv(gd_t gd, double *ip, double *vp);

char *prog = "";

static const char *options = OPTIONS_COMMON "lciSI:V:o:s:p:F:";

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    OPTIONS_COMMON_LONG,
    {"address",         required_argument, 0, 'a'},
    {"verbose",         no_argument,       0, 'v'},
    {"local",           no_argument,       0, 'l'},
    {"clear",           no_argument,       0, 'c'},
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

static opt_desc_t optdesc[] = {
    OPTIONS_COMMON_DESC,
    { "l", "local", "return instrument to front panel control" },
    { "c", "clear", "initialize instrument to default values" },
    { "i", "get-idn", "print instrument model" },
    { "S", "selftest", "run selftest" },
    { "I", "iset", "set current" },
    { "V", "vset", "set voltage" },
    { "o", "out", "enable/disable output (0|1)" },
    { "F", "foldback", "set foldback (off|cv|cc)" },
    { "s", "samples", "number of samples [0]" },
    { "p", "period", "sample period" },
    { 0, 0, 0 },
};

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

    gd = gpib_init_args(argc, argv, options, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage)
        usage(optdesc);
    if (!gd)
        exit(1);

    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            OPTIONS_COMMON_SWITCH
                break;
            case 'l': /* --local */
                gpib_loc(gd);
                break;
            case 'c': /* --clear */
                gpib_clr(gd, 1000000); /* same as 'CLR' command */
                gpib_wrtf(gd, "OUT 0;RST\n");
                sleep(1);
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
                    fprintf(stderr, "%s: error parsing period arg\n", prog);
                    fprintf(stderr, "%s: use units of: %s\n", 
                            prog, PERIOD_UNITS);
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
