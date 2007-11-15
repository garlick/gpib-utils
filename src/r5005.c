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

#define _GNU_SOURCE /* for asprintf */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include <assert.h>
#include <sys/time.h>

#include "r5005.h"
#include "units.h"
#include "gpib.h"
#include "util.h"

#define INSTRUMENT "r5005"

#define IGNORE_43   (0)     /* suppress 'error 43' - XXX do we see this now? */

static strtab_t _y_errors[] = Y_ERRORS;

char *prog = "";

#define OPTIONS "n:clvf:r:t:H:s:T:"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"function",        required_argument, 0, 'f'},
    {"range",           required_argument, 0, 'r'},
    {"trigger",         required_argument, 0, 't'},
    {"highres",         required_argument, 0, 'H'},
    {"samples",         required_argument, 0, 's'},
    {"period",          required_argument, 0, 'T'},
    {0, 0, 0, 0},
};

static void 
usage(void)
{
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name                            instrument address [%s]\n"
"  -c,--clear                                clear dmm, set defaults\n"
"  -l,--local                                enable front panel\n"
"  -v,--verbose                              show protocol on stderr\n"
"  -f,--function dcv|acv|kohm                select function [dcv]\n"
"  -r,--range 0.1|1|10|100|1k|10k|auto       select range [auto]\n"
"  -t,--trigger int|ext|hold|extdly|holddly  select trigger mode [int]\n"
"  -H,--highres off|on                       select high res [off]\n"
"  -s,--samples                              number of samples [0]\n"
"  -T,--period                               sample period in ms [0]\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

static double
_gettime(void)
{
    struct timeval t;

    if (gettimeofday(&t, NULL) < 0) {
        perror("gettimeofday");
        exit(1);
    }
    return ((double)t.tv_sec + (double)t.tv_usec / 1000000);
}

static void
_sleep_sec(double sec)
{
    if (sec > 0)
        usleep((unsigned long)(sec * 1000000.0));
}

static void 
r5005_checksrq(gd_t gd)
{
    unsigned char status, error = 0;
    int count, rdy = 0;

    gpib_rsp(gd, &status);
    if ((status & R5005_STAT_SRQ)) {
        if (status & R5005_STAT_DATA_READY)
            rdy = 1;
        if (status & R5005_STAT_ERROR_AVAIL) {
            gpib_wrtstr(gd, R5005_CMD_ERROR_XMIT); /* get Y error */
            count = gpib_rd(gd, &error, 1);
            if (count != 1) {
                fprintf(stderr, "%s: error reading error byte\n", prog);
                exit(1);
            }
            if ((IGNORE_43) && error != 43) { /* error 43 seems innocuous... */
                fprintf(stderr, "%s: srq: error %d: %s\n", 
                        prog, error, findstr(_y_errors, error));
                exit(1);
            }
        }
        /* Note: R5005_STAT_ABNORMAL follows R5005_STAT_ERROR_AVAIL,
         * so we don't handle it separately 
         */
        /* Note: R5005_STAT_SIG_INTEGRAT is not an error.
         * It means the dmm is in the process of integrating. 
         */
    }
}

int
main(int argc, char *argv[])
{
    int verbose = 0;
    int clear = 0;
    int local = 0;
    int showtime = 0;
    char *addr = NULL;
    gd_t gd;
    int c;
    char *function = NULL;
    char *range = NULL;
    char *trigger = NULL;
    char *highres = NULL;
    int samples = 0;
    double period = 0;
    double freq;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'n': /* --name */
            addr = optarg;
            break;
        case 'c': /* --clear */
            clear = 1;
            break;
        case 'l': /* --local */
            local = 1;
            break;
        case 'v': /* --verbose */
            verbose = 1;
            break;
        case 'T': /* --period */
            if (freqstr(optarg, &freq) < 0) {
                fprintf(stderr, "%s: error parsing period argument\n", prog);
                fprintf(stderr, "%s: use freq units: %s\n", prog, FREQ_UNITS);
                fprintf(stderr, "%s: or period units: %s\n", prog,PERIOD_UNITS);
                exit(1);
            }
            period = 1.0/freq;
            break;
        case 's': /* --samples */
            samples = (int)strtoul(optarg, NULL, 10);
            if (samples > 1)
                showtime = 1;
            break;
        case 'H': /* --highres */
            if (strcasecmp(optarg, "off") == 0)
                highres = R5005_HIGHRES_OFF;
            else if (strcasecmp(optarg, "on") == 0)
                highres = R5005_HIGHRES_ON;
            else
                usage();
            break;
        case 't': /* --trigger */
            if (strcasecmp(optarg, "int") == 0)
                trigger = R5005_TRIGGER_INT;
            else if (strcasecmp(optarg, "ext") == 0)
                trigger = R5005_TRIGGER_EXT;
            else if (strcasecmp(optarg, "hold") == 0)
                trigger = R5005_TRIGGER_HOLD;
            else if (strcasecmp(optarg, "extdly") == 0)
                trigger = R5005_TRIGGER_EXT_DELAY;
            else if (strcasecmp(optarg, "holddly") == 0)
                trigger = R5005_TRIGGER_HOLD_DELAY;
            else
                usage();
            break;
        case 'r': /* --range */
            if (strcasecmp(optarg, "0.1") == 0)
                range = R5005_RANGE_0_1;
            else if (strcasecmp(optarg, "1") == 0)
                range = R5005_RANGE_1;
            else if (strcasecmp(optarg, "10") == 0)
                range = R5005_RANGE_10;
            else if (strcasecmp(optarg, "100") == 0)
                range = R5005_RANGE_100;
            else if (strcasecmp(optarg, "1K") == 0)
                range = R5005_RANGE_1K;
            else if (strcasecmp(optarg, "10K") == 0)
                range = R5005_RANGE_10K;
            else if (strcasecmp(optarg, "auto") == 0)
                range = R5005_RANGE_AUTO;
            else
                usage();
            break;
        case 'f': /* --function */
            if (strcasecmp(optarg, "dcv") == 0)
                function = R5005_FUNC_DCV;
            else if (strcasecmp(optarg, "acv") == 0)
                function = R5005_FUNC_ACV;
            else if (strcasecmp(optarg, "kohm") == 0)
                function = R5005_FUNC_KOHMS;
            else
                usage();
            break;
        default:
            usage();
            break;
        }
    }
    if (optind < argc)
        usage();

    if (!clear && !local && !function && !range && !trigger && !highres
            && samples == 0)
        usage();

    if (!addr)
        addr = gpib_default_addr(INSTRUMENT);
    if (!addr) {
        fprintf(stderr, "%s: use --name to provide instrument address\n", prog);
        exit(1);
    }

    gd = gpib_init(addr, NULL, 0);
    if (!gd) {
        fprintf(stderr, "%s: device initialization failed for address %s\n", 
                prog, addr);
        exit(1);
    }
    gpib_set_verbose(gd, verbose);
    gpib_set_reos(gd, 1);

    /* clear dmm state */
    if (clear) {
        gpib_clr(gd, 100000);
        r5005_checksrq(gd);
        gpib_wrtf(gd, "%s", R5005_CMD_INIT);
        r5005_checksrq(gd);
    }

    /* configure dmm */
    if (function) {
        gpib_wrtf(gd, "%s", function);
        r5005_checksrq(gd);
    }
    if (range) {
        gpib_wrtf(gd, "%s", range);
        r5005_checksrq(gd);
    }
    if (trigger) {
        gpib_wrtf(gd, "%s", trigger);
        r5005_checksrq(gd);
    }
    if (highres) {
        gpib_wrtf(gd, "%s", highres);
        r5005_checksrq(gd);
    }

    r5005_checksrq(gd);

    /* take some readings and send to stdout */
    if (samples > 0) {
        char buf[1024];
        double t0, t1, t2;

        t0 = 0;
        while (samples-- > 0) {
            t1 = _gettime();
            if (t0 == 0)
                t0 = t1;
            gpib_rdstr(gd, buf, sizeof(buf));
            r5005_checksrq(gd);
            t2 = _gettime();

            /* Output suitable for gnuplot:
             *   t(s)  sample 
             * sample already has a crlf terminator
             */
            if (showtime)
                printf("%-3.3lf\t%s", (t1-t0), buf);
            else
                printf("%s", buf);

            if (samples > 0 && period > 0)
                _sleep_sec(period - (t2-t1));
        }
    }

    /* give back the front panel */
    if (local) {
        gpib_loc(gd); 
        r5005_checksrq(gd);
    }

    gpib_fini(gd);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
