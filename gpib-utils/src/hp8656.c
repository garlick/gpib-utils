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
#include <math.h>
#include <gpib/ib.h>

#include "hp8656.h"
#include "units.h"

#define INSTRUMENT "hp8656" /* the /etc/gpib.conf entry */
#define BOARD       0       /* minor board number in /etc/gpib.conf */

static char *prog = "";
static int verbose = 0;

#define OPTIONS "n:f:a:vclF:A:"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"incrfreq",        required_argument, 0, 'F'},
    {"frequency",       required_argument, 0, 'f'},
    {"incrampl",        required_argument, 0, 'A'},
    {"amplitude",       required_argument, 0, 'a'},
    {0, 0, 0, 0},
};

static void 
usage(void)
{
    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name                instrument name [%s]\n"
"  -c,--clear                    initialize instrument to default values\n"
"  -l,--local                    return instrument to local operation on exit\n"
"  -v,--verbose                  show protocol on stderr\n"
"  -f,--frequency [value|up|dn]  set carrier freq [100Mhz]\n"
"  -a,--amplitude [value|up|dn]  set carrier amplitude [-127dBm]\n"
"  -F,--incrfreq value           set carrier freq increment [10.0MHz]\n"
"  -A,--incrampl value           set carrier amplitude increment [10.0dB]\n"
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
        if (n == -1) {
            fprintf(stderr, "%s: out of memory\n", prog);
            exit(1);
        }
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
    int clear = 0;
    int local = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'n': /* --name */
            instrument = optarg;
            break;
        case 'c': /* --clear */
            clear = 1;
            break;
        case 'l': /* --local */
            local = 1;
            break;
        case 'F': /* --incrfreq */
            if (freqstr(optarg, &f) < 0) {
                fprintf(stderr, "%s: error parsing incrfreq arg\n", prog);
                exit(1);
            }
            if (f > 989.99E6 || f < 0.1E3) {
                fprintf(stderr, 
                        "%s: incrfreq out of range (0.1kHz-989.99MHz)\n", prog);
                exit(1);
            }
            if (fmod(f, 100) != 0 && fmod(f, 250) != 0) {
                fprintf(stderr, 
                        "%s: incrfreq must be a multiple of 100Hz or 250Hz\n",
                        prog);
                exit(1);
            }
            sprintf(cmdstr + strlen(cmdstr), "%s%s%.0lf%s", 
                HP8656_FREQ, HP8656_INCR_SET, f, HP8656_UNIT_HZ);
            break;
        case 'f': /* --frequency */
            if (!strcasecmp(optarg, "up")) {
                sprintf(cmdstr + strlen(cmdstr), "%s%s", 
                        HP8656_FREQ, HP8656_STEP_UP);
            } else if (!strcasecmp(optarg, "dn") ||!strcasecmp(optarg, "down")){
                sprintf(cmdstr + strlen(cmdstr), "%s%s", 
                        HP8656_FREQ, HP8656_STEP_DN); 
            } else {
                if (freqstr(optarg, &f) < 0) {
                    fprintf(stderr, "%s: error parsing frequency arg\n", prog);
                    exit(1);
                }
                if (f < 100E3 || f > 990E6) {
                    fprintf(stderr, "%s: freq out of range (100kHz-990MHz)\n",prog);
                    exit(1);
                }
                sprintf(cmdstr + strlen(cmdstr), "%s%.0lf%s", 
                        HP8656_FREQ, f, HP8656_UNIT_HZ);
            }
            break;
        case 'A': /* --incrampl */
            /* XXX instrument accepts '%' units here too */
            if (amplstr(optarg, &a) < 0)  {
                char *end;
                double a = strtod(optarg, &end);

                if (strcasecmp(end, "db") != 0) {
                    fprintf(stderr, "%s: error parsing incrampl arg\n", prog);
                    exit(1);
                }
                if (a < 0.1 || a > 144.0) {
                    fprintf(stderr, 
                            "%s: incrampl out of range (0.1dB-144.0db)\n", 
                            prog);
                    exit(1);
                }
                sprintf(cmdstr + strlen(cmdstr), "%s%s%.2lf%s", 
                    HP8656_AMPL, HP8656_INCR_SET, a, HP8656_UNIT_DB);
            } else {
                /* XXX instrument ignored minus sign when dbm units used here */
                a = dbmtov(a);  /* convert dbm to volts */
                if (a < 0.01E-6 || a > 1.57) {
                    fprintf(stderr, 
                            "%s: incrampl out of range (0.01uV-1.57V)\n", prog);
                    exit(1);
                }
                sprintf(cmdstr + strlen(cmdstr), "%s%s%.2lf%s", 
                    HP8656_AMPL, HP8656_INCR_SET, a*1E6, HP8656_UNIT_UV);
            }
            break;
        case 'a': /* --amplitude */
            if (!strcasecmp(optarg, "up")) {
                sprintf(cmdstr + strlen(cmdstr), "%s%s", 
                        HP8656_AMPL, HP8656_STEP_UP);
            } else if (!strcasecmp(optarg, "dn") ||!strcasecmp(optarg, "down")){
                sprintf(cmdstr + strlen(cmdstr), "%s%s", 
                        HP8656_AMPL, HP8656_STEP_DN); 
            } else {
                if (amplstr(optarg, &a) < 0) {
                    fprintf(stderr, "%s: error parsing amplitude arg\n", prog);
                    exit(1);
                }
                if (a > 17.0) {
                    fprintf(stderr, "%s: amplitude exceeds range\n", prog);
                    exit(1);
                }
                if (a < -127.0 || a > 13.0)
                    fprintf(stderr, "%s: warning: amplitude beyond cal\n", 
                            prog);
                sprintf(cmdstr + strlen(cmdstr), "%s%.2lf%s", 
                        HP8656_AMPL, a, HP8656_UNIT_DBM);
            }
            break;
        case 'v': /* --verbose */
            verbose = 1;
            break;
        default:
            usage();
            break;
        }
    }

    if (!clear && !*cmdstr && !local)
        usage();

    /* find device in /etc/gpib.conf */
    d = ibfind(instrument);
    if (d < 0) {
        fprintf(stderr, "%s: not found: %s\n", prog, instrument);
        exit(1);
    }

    /* clear instrument to default settings */
    if (clear) {
        _ibclr(d);
        sleep(1); /* instrument won't respond for about 1s after clear */
    }

    /* write cmd if specified */
    if (strlen(cmdstr) > 0)
        _ibwrtf(d, "%s", cmdstr);

    /* Sleep 2s to accomodate worst case settling time.
     * FIXME: The actual settling time should be calculated based 
     * on info in the manual.
     */
    if (cmdstr || clear)
        sleep(2);

    /* return front panel if requested */
    if (local)
        _ibloc(d); 

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
