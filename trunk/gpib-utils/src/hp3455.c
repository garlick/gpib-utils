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

#include "hp3455.h"

#define INSTRUMENT "hp3455" /* the /etc/gpib.conf entry */
#define BOARD       0       /* minor board number in /etc/gpib.conf */

char *prog = "";

#define OPTIONS "n:f:r:t:m:a:H:s:T:y:z:"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
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
"  -f,--function dcv|acv|facv|kohm2|kohm4|test    select function [dcv]\n"
"  -r,--range 0.1|1|10|100|1k|10k|auto            select range [auto]\n"
"  -t,--trigger int|ext|hold                      select trigger mode [int]\n"
"  -a,--autocal off|on                            select auto cal mode [on]\n"
"  -H,--highres off|on                            select high res [off]\n"
"  -m,--math scale|err|off                        select math mode [off]\n"
"  -y,--storey value                              store value into y reg\n"
"  -y,--storez value                              store value into z reg\n"
"  -s,--samples                                   number of samples [1]\n"
"  -T,--period                                    sample period in ms [0]\n"
           , prog, INSTRUMENT);
    exit(1);
}

static unsigned long _gettime_ms(void)
{
    struct timeval t;

    if (gettimeofday(&t, NULL) < 0) {
        perror("gettimeofday");
        exit(1);
    }
    return (t.tv_sec * 1000 + t.tv_usec / 1000);
}

static void _sleep_ms(long ms)
{
    if (ms > 0)
        usleep(ms * 1000L);
}

/* Warning: ibrsp has the side effect of modifying ibcnt! */
static void _checksrq(int d, int *rdyp)
{
    char status;
    int rdy = 0;

    ibrsp(d, &status);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibrsp error %d\n", prog, iberr);
        exit(1);
    }
    if ((status & HP3455_STAT_SRQ)) {
        if (status & HP3455_STAT_DATA_READY)
            rdy = 1;
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
    if (rdyp)
        *rdyp = rdy;    
}

static void _ibtrg(int d)
{
    ibtrg(d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibtrg error %d\n", prog, iberr);
        exit(1);
    }
    _checksrq(d, NULL);
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
    _checksrq(d, NULL);
}

static void _ibwrtstr(int d, char *str)
{
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
    _checksrq(d, NULL);
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

static void _ibrdstr(int d, char *buf, int len)
{
    ibrd(d, buf, len - 1);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibrd timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibrd error %d\n", prog, iberr);
        exit(1);
    }
    assert(ibcnt >=0 && ibcnt < len);
    buf[ibcnt] = '\0';
    _checksrq(d, NULL);
}

static void _ibloc(int d)
{
    ibloc(d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibloc error %d\n", prog, iberr);
        exit(1);
    }
    _checksrq(d, NULL);
}

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int d, c;
    char *function =    HP3455_FUNC_DC;
    char *range =       HP3455_RANGE_AUTO;
    char *trigger =     HP3455_TRIGGER_INT;
    char *math =        HP3455_MATH_OFF;
    char *autocal =     HP3455_AUTOCAL_ON;
    char *highres =     HP3455_HIGHRES_OFF;
    char *ready_rqs =   HP3455_DATA_RDY_RQS_ON;
    int samples = 1;
    long period_ms = 0;
    double y = 0.0, z = 0.0;
    int storey = 0, storez = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'y': /* --storey */
            y = strtod(optarg, NULL);
            storey = 1;
            break;
        case 'z': /* --storez */
            z = strtod(optarg, NULL);
            storez = 1;
            break;
        case 'T': /* --period */
            period_ms = strtoul(optarg, NULL, 10);
            break;
        case 's': /* --samples */
            samples = (int)strtoul(optarg, NULL, 10);
            break;
        case 'n': /* --name */
            instrument = optarg;
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

    /* find device in /etc/gpib.conf */
    d = ibfind(instrument);
    if (d < 0) {
        fprintf(stderr, "%s: not found: %s\n", prog, instrument);
        exit(1);
    }

    /* clear dmm state */
    _ibclr(d);

    /* configure dmm */
    _ibwrtf(d, "%s%s%s%s%s%s%s", 
            function, range, trigger, math, autocal, highres, ready_rqs);

    /* store y and z register values */
    if (storey)
        _ibwrtf(d, "%s %f %s", HP3455_ENTER_Y, y, HP3455_STORE_Y);
    if (storez)
        _ibwrtf(d, "%s %f %s", HP3455_ENTER_Z, z, HP3455_STORE_Z);

    /* take some readings and send to stdout */
    if (samples > 0) {
        char buf[1024];
        long t0, t1, t2;
        int ready;
        short s;

        t0 = 0;
        while (samples-- > 0) {
            t1 = _gettime_ms();
            if (t0 == 0)
                t0 = t1;
 
            if (period_ms > 0)
                _ibtrg(d);
            else if (strcmp(trigger, HP3455_TRIGGER_HOLD_MAN) == 0)
                _ibloc(d);  /* allow hold/manual button to work */

            /* Block until SRQ and serial poll indicates data available.
             */
            ready = 0;
            while (!ready) {
                WaitSRQ(BOARD, &s);
                if (s == 1) /* 1 = srq, 0 = timeout */
                    _checksrq(d, &ready);
            } 

            _ibrdstr(d, buf, sizeof(buf));
            t2 = _gettime_ms();

            /* Output suitable for gnuplot:
             *   t(s)  sample 
             * sample already has a crlf terminator
             */
            printf("%ld.%-3.3ld\t%s", (t1-t0)/1000, (t1-t0)%1000, buf); 

            if (samples > 0 && period_ms > 0)
                _sleep_ms(period_ms - (t2-t1));
        }
    }

    /* give back the front panel */
    _ibloc(d); 

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
