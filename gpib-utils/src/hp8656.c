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
#include <gpib/ib.h>

#include "hp8656.h"

#define INSTRUMENT "hp8656" /* the /etc/gpib.conf entry */
#define BOARD       0       /* minor board number in /etc/gpib.conf */

extern double freqstr(char *str);
extern int amplstr(char *str, double *a);

static char *prog = "";
static int verbose = 0;

#define OPTIONS "n:f:a:v"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"frequency",       required_argument, 0, 'f'},
    {"amplitude",       required_argument, 0, 'a'},
    {"verbose",         no_argument,       0, 'v'},
    {0, 0, 0, 0},
};

static void 
usage(void)
{
    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name                            instrument name [%s]\n"
"  -f,--frequency                            set carrier freq [100Mhz]\n"
"  -a,--amplitude                            set carrier amplitude [-127dBm]\n"
"  -v,--verbose                              show protocol on stderr\n"
           , prog, INSTRUMENT);
    exit(1);
}


static void _ibclr(int d)
{
    ibclr(d);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibclr timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibclr error %d\n", prog, iberr);
        exit(1);
    }
}

static void _ibwrtstr(int d, char *str)
{
    if (verbose)
        fprintf(stderr, "T: %s\n", str);
    ibwrt(d, str, strlen(str));
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibwrt timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibwrt error %d\n", prog, iberr);
        exit(1);
    }
    if (ibcnt != strlen(str)) {
        fprintf(stderr, "%s: ibwrt: short write\n", prog);
        exit(1);
    }
}

static void _ibwrtf(int d, char *fmt, ...)
{
        va_list ap;
        char *s;
        int n;

        va_start(ap, fmt);
        n = vasprintf(&s, fmt, ap);
        va_end(ap);
        _ibwrtstr(d, s);
        free(s);
}

static void _ibloc(int d)
{
    ibloc(d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibloc error %d\n", prog, iberr);
        exit(1);
    }
}

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    char cmdstr[1024] = "";
    double f, a;
    int c, d;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'n': /* --name */
            instrument = optarg;
            break;
        case 'f': /* --frequency */
            f = freqstr(optarg);
            if (f < 0) {
                fprintf(stderr, "%s: can't parse freq: %s\n", prog, optarg);
                exit(1);
            }
            if (f < 100E3 || f > 990E6) {
                fprintf(stderr, "%s: freq out of range (100kHz-990MHz)\n",prog);
                exit(1);
            }
            sprintf(cmdstr + strlen(cmdstr), "%s%.0lf%s", 
                    HP8656_FREQ, f, HP8656_UNIT_HZ);
            break;
        case 'a': /* --amplitude */
            if (amplstr(optarg, &a) < 0) {
                fprintf(stderr, "%s: can't parse ampl: %s\n", prog, optarg);
                exit(1);
            }
            if (a > 17.0) {
                fprintf(stderr, 
                        "%s: amplitude of %le dBm exceeds range of instrument", 
                       prog, a);
                exit(1);
            }
            if (a < -127.0 || a > 13.0) {
                fprintf(stderr, 
                        "%s: warning: amplitude of %le dBm outside of cal\n", 
                        prog, a);
            }
            sprintf(cmdstr + strlen(cmdstr), "%s%.2lf%s", 
                    HP8656_AMPL, a, HP8656_UNIT_DBM);
            break;
        case 'v': /* --verbose */
            verbose = 1;
            break;
        default:
            usage();
            break;
        }
    }

    /* find device in /etc/gpib.conf */
    d = ibfind(instrument);
    if (d < 0) {
        fprintf(stderr, "%s: not found: %s\n", prog, instrument);
        exit(1);
    }

    /* Clear message - sets instrument to the following state:
     *   carrier freq:             100.0000 MHz
     *   carrier ampl:             -127.0 dBm
     *   AM depth:                 0%
     *   FM peak deviation freq:   0.0 kHz
     *   carrier freq incr:        10.0000 MHz
     *   carrier ampl incr:        10.0 dB
     *   AM depth incr:            1%
     *   FM peak deviation incr:   1.0 kHz
     *   coarse and fine tune ptr: 10.0000 MHz
     *   seq counter:              0
     *   10 internal storage regs: unchanged
     */
    _ibclr(d);

    sleep(1);

    /* write new configuration if specified */
    if (strlen(cmdstr) > 0)
        _ibwrtf(d, "%s", cmdstr);

    /* give back the front panel */
    _ibloc(d); 

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
