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

#include "gpib.h"
#include "hp3455.h"
#include "units.h"

#define INSTRUMENT "hp3455" /* the /etc/gpib.conf entry */
#define BOARD       0       /* minor board number in /etc/gpib.conf */

char *prog = "";

#define OPTIONS "n:clvf:r:t:m:a:H:s:T:y:z:"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"function",        required_argument, 0, 'f'},
    {"range",           required_argument, 0, 'r'},
    {"trigger",         required_argument, 0, 't'},
    {"math",            required_argument, 0, 'm'},
    {"autocal",         required_argument, 0, 'a'},
    {"highres",         required_argument, 0, 'H'},
    {"samples",         required_argument, 0, 's'},
    {"period",          required_argument, 0, 'T'},
    {"storey",          required_argument, 0, 'y'},
    {"storez",          required_argument, 0, 'z'},
    {0, 0, 0, 0},
};

static void 
usage(void)
{
    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name                                 dmm name [%s]\n"
"  -c,--clear                                     clear instrument\n"
"  -l,--local                                     re-enable front panel\n"
"  -v,--verbose                                   watch protocol on stderr\n"
"  -f,--function dcv|acv|facv|kohm2|kohm4|test    select function\n"
"  -r,--range 0.1|1|10|100|1k|10k|auto            select range\n"
"  -t,--trigger int|ext|hold                      select trigger mode\n"
"  -a,--autocal off|on                            select auto cal mode\n"
"  -H,--highres off|on                            select high res\n"
"  -m,--math scale|err|off                        select math mode\n"
"  -y,--storey value                              store value into y reg\n"
"  -y,--storez value                              store value into z reg\n"
"  -s,--samples                                   number of samples [0]\n"
"  -T,--period                                    sample period in ms [0]\n"
           , prog, INSTRUMENT);
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


/* Warning: ibrsp has the side effect of modifying ibcnt! */
static void 
hp3455_checksrq(int d)
{
    char status;

    gpib_ibrsp(d, &status);
    if ((status & HP3455_STAT_SRQ)) {
        if (status & HP3455_STAT_DATA_READY) {
        }
        if (status & HP3455_STAT_SYNTAX_ERR) {
            fprintf(stderr, "%s: srq: syntax error\n", prog);
            exit(1);
        }
        if (status & HP3455_STAT_BINARY_ERR) {
            fprintf(stderr, "%s: srq: binary function error\n", prog);
            exit(1);
        }
        if (status & HP3455_STAT_TOOFAST_ERR) {
            fprintf(stderr, "%s: srq: too fast error\n", prog);
            exit(1);
        }
    }
}

int
main(int argc, char *argv[])
{
    int verbose = 0;
    int clear = 0;
    int local = 0;
    int showtime = 0;
    char *instrument = INSTRUMENT;
    int d, c;
    char *function = NULL;
    char *range = NULL;
    char *trigger = NULL;
    char *math = NULL;
    char *autocal = NULL;
    char *highres = NULL;
    int samples = 0;
    double period = 0;
    double freq;
    double y = 0.0, z = 0.0;
    int storey = 0, storez = 0;

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
        case 'v': /* --verbose */
            verbose = 1;
            break;
        case 'y': /* --storey */
            y = strtod(optarg, NULL);
            storey = 1;
            break;
        case 'z': /* --storez */
            z = strtod(optarg, NULL);
            storez = 1;
            break;
        case 'T': /* --period */
            if (freqstr(optarg, &freq) < 0) {
                fprintf(stderr, "%s: error parsing period argument\n", prog);
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
                highres = HP3455_HIGHRES_OFF;
            else if (strcasecmp(optarg, "on") == 0)
                highres = HP3455_HIGHRES_ON;
            else
                usage();
            break;
        case 'a': /* --autocal */
            if (strcasecmp(optarg, "off") == 0)
                autocal = HP3455_AUTOCAL_OFF;
            else if (strcasecmp(optarg, "on") == 0)
                autocal = HP3455_AUTOCAL_ON;
            else
                usage();
            break;
        case 'm': /* --math */
            if (strcasecmp(optarg, "scale") == 0)
                math = HP3455_MATH_SCALE;
            else if (strcasecmp(optarg, "err") == 0)
                math = HP3455_MATH_ERROR;
            else if (strcasecmp(optarg, "off") == 0)
                math = HP3455_MATH_OFF;
            else
                usage();
            break;
        case 't': /* --trigger */
            if (strcasecmp(optarg, "int") == 0)
                trigger = HP3455_TRIGGER_INT;
            else if (strcasecmp(optarg, "ext") == 0)
                trigger = HP3455_TRIGGER_EXT;
            else if (strcasecmp(optarg, "hold") == 0)
                trigger = HP3455_TRIGGER_HOLD_MAN;
            else
                usage();
            break;
        case 'r': /* --range */
            if (strcasecmp(optarg, "0.1") == 0)
                range = HP3455_RANGE_0_1;
            else if (strcasecmp(optarg, "1") == 0)
                range = HP3455_RANGE_1;
            else if (strcasecmp(optarg, "10") == 0)
                range = HP3455_RANGE_10;
            else if (strcasecmp(optarg, "100") == 0)
                range = HP3455_RANGE_100;
            else if (strcasecmp(optarg, "1K") == 0)
                range = HP3455_RANGE_1K;
            else if (strcasecmp(optarg, "10K") == 0)
                range = HP3455_RANGE_10K;
            else if (strcasecmp(optarg, "auto") == 0)
                range = HP3455_RANGE_AUTO;
            else
                usage();
            break;
        case 'f': /* --function */
            if (strcasecmp(optarg, "dcv") == 0)
                function = HP3455_FUNC_DC;
            else if (strcasecmp(optarg, "acv") == 0)
                function = HP3455_FUNC_AC;
            else if (strcasecmp(optarg, "facv") == 0)
                function = HP3455_FUNC_FAST_AC;
            else if (strcasecmp(optarg, "kohm2") == 0)
                function = HP3455_FUNC_2WIRE_KOHMS;
            else if (strcasecmp(optarg, "kohm4") == 0)
                function = HP3455_FUNC_4WIRE_KOHMS;
            else if (strcasecmp(optarg, "test") == 0)
                function = HP3455_FUNC_TEST;
            else
                usage();
            break;
        default:
            usage();
            break;
        }
    }

    if (!clear && !local && !function && !range && !autocal && !math 
            && !trigger && !autocal && !highres && !storey && !storez 
            && samples == 0)
        usage();

    /* find device in /etc/gpib.conf */
    d = gpib_init(prog, instrument, verbose);

    /* clear dmm state */
    if (clear) { 
        gpib_ibclr(d);
        hp3455_checksrq(d);
    }

    /* configure dmm */
    if (function) {
        gpib_ibwrtf(d, "%s", function);
        hp3455_checksrq(d);
    }
    if (range) {
        gpib_ibwrtf(d, "%s", range);
        hp3455_checksrq(d);
    }
    if (trigger) {
        gpib_ibwrtf(d, "%s", trigger);
        hp3455_checksrq(d);
    }
    if (math) {
        gpib_ibwrtf(d, "%s", math);
        hp3455_checksrq(d);
    }
    if (autocal) {
        gpib_ibwrtf(d, "%s", autocal);
        hp3455_checksrq(d);
    }
    if (highres) {
        gpib_ibwrtf(d, "%s", highres);
        hp3455_checksrq(d);
    }

    /* store y and z register values */
    if (storey) {
        gpib_ibwrtf(d, "%s %f %s", HP3455_ENTER_Y, y, HP3455_STORE_Y);
        hp3455_checksrq(d);
    }
    if (storez) {
        gpib_ibwrtf(d, "%s %f %s", HP3455_ENTER_Z, z, HP3455_STORE_Z);
        hp3455_checksrq(d);
    }

    /* take some readings and send to stdout */
    if (samples > 0) {
        char buf[1024];
        double t0, t1, t2;

        t0 = 0;
        while (samples-- > 0) {
            t1 = _gettime();
            if (t0 == 0)
                t0 = t1;
 
            gpib_ibrdstr(d, buf, sizeof(buf));
            hp3455_checksrq(d);
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
    if (local)
        gpib_ibloc(d); 

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
