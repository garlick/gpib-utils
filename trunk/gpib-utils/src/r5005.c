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

#include "r5005.h"

#define INSTRUMENT "r5005" /* the /etc/gpib.conf entry */
#define BOARD       0       /* minor board number in /etc/gpib.conf */

#define IGNORE_43   (0)     /* suppress 'error 43' - XXX do we see this now? */

typedef struct {
    int     num;
    char   *str;
} errstr_t;

static void _ibwrtstr_nochecksrq(int d, char *str);
static int _ibrd(int d, char *buf, int len, int nochecksrq);

char *prog = "";

#define OPTIONS "n:f:r:t:H:s:T:"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
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
    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name                                 dmm name [%s]\n"
"  -f,--function dcv|acv|kohm                     select function [dcv]\n"
"  -r,--range 0.1|1|10|100|1k|10k|auto            select range [auto]\n"
"  -t,--trigger int|ext|hold|extdly|holddly       select trigger mode [int]\n"
"  -H,--highres off|on                            select high res [off]\n"
"  -s,--samples                                   number of samples [1]\n"
"  -T,--period                                    sample period in ms [0]\n"
           , prog, INSTRUMENT);
    exit(1);
}

/* "Y" error codes */
static errstr_t _y_errors[] = {
    { R5005_ERR_PERCENT_CONST,      
        "percent constant: 0 during percent calculations" },
    { R5005_ERR_ZERO_NOT_DC_0_1,    
        "digital zero: not in the DC function, 0.1 range" },
    { R5005_ERR_ZERO_NOSHORT,
        "digital zero: input not shorted" },
    { R5005_ERR_BADRAM_U35,    
        "defective RAM (U35)" },
    { R5005_ERR_NVRAM,    
        "NVRAM contents disrupted, need to verify calibration" },
    { R5005_ERR_ZERO_BIGOFFSET,    
        "digital zero: digitizer offset >1000 digits was measured" },
    { R5005_ERR_PERCENT_DEVIATION,    
        "percent function: percent deviation >= 10^10%%" },
    { R5005_ERR_BADRAM_U22_U31,    
        "defective RAM (U22 and/or U31)" },
    { R5005_ERR_STORE,    
        "register store: overload reading or time >99.5959 hours" },
    { R5005_ERR_RECALL,    
        "register recall: program setting isn't stored yet" },
    { R5005_ERR_RECALL_EMPTY,    
        "register recall: no readings taken yet (gpib)" },
    { R5005_ERR_TRIGGER_TOO_FAST,    
        "triggered too fast or too often (gpib)" },
    { R5005_ERR_SYNTAX,    
        "syntax error during programming (gpib)" },
    { R5005_ERR_OPTION,    
        "option not installed (gpib)" },
    { 0, NULL },
};

static char *_finderr(errstr_t *tab, int num)
{
    errstr_t *tp;

    for (tp = &tab[0]; tp->str != NULL; tp++)
        if (tp->num == num)
            break;

    return tp->str ? tp->str : "unknown error";
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
static void _spoll(int d, int *rdyp)
{
    char status, error = 0;
    int count, rdy = 0;

    ibrsp(d, &status);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibrsp timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibrsp error %d\n", prog, iberr);
        exit(1);
    }
    if ((status & R5005_STAT_SRQ)) {
        if (status & R5005_STAT_DATA_READY)
            rdy = 1;
        if (status & R5005_STAT_ERROR_AVAIL) {
            _ibwrtstr_nochecksrq(d, R5005_CMD_ERROR_XMIT); /* get Y error */
            count = _ibrd(d, &error, 1, 1);
            if (count != 1) {
                fprintf(stderr, "%s: error reading error byte\n", prog);
                exit(1);
            }
            if ((IGNORE_43) && error != 43) { /* error 43 seems innocuous... */
                fprintf(stderr, "%s: srq: error %d: %s\n", 
                        prog, error, _finderr(_y_errors, error));
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
    if (rdyp)
        *rdyp = rdy;    
}

static void _checksrq(int d)
{
    /* XXX making this conditional on (ibsta & RQS) seems to cause a hang...
     */
    _spoll(d, NULL);
}

static void _ibtrg(int d)
{
    ibtrg(d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibtrg error %d\n", prog, iberr);
        exit(1);
    }
#if 0
    /* XXX This check is apparently bad here - ibrsp occasionally fails.
     */
    _checksrq(d);
#endif
}

static void _ibwrt(int d, char *str, int len, int nochecksrq)
{
    ibwrt(d, str, len);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibwrt timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibwrt error %d\n", prog, iberr);
        exit(1);
    }
    if (ibcnt != len) {
        fprintf(stderr, "%s: ibwrt: short write\n", prog);
        exit(1);
    }
    if (!nochecksrq)
        _checksrq(d);
}

static void _ibwrtstr(int d, char *str)
{
    _ibwrt(d, str, strlen(str), 0);
}

static void _ibwrtstr_nochecksrq(int d, char *str)
{
    _ibwrt(d, str, strlen(str), 1);
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

static int _ibrd(int d, char *buf, int len, int nochecksrq)
{
    int count;

    ibrd(d, buf, len);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibrd timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibrd error %d\n", prog, iberr);
        exit(1);
    }
    assert(ibcnt >=0 && ibcnt <= len);
    count = ibcnt;

    if (!nochecksrq)
        _checksrq(d);

    return count;
}

static void _ibrdstr(int d, char *buf, int len)
{
    int count;
    
    count = _ibrd(d, buf, len - 1, 0);
    assert(count < len);
    buf[count] = '\0';
}

static void _ibloc(int d)
{
    ibloc(d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibloc error %d\n", prog, iberr);
        exit(1);
    }
    _checksrq(d);
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
    _sleep_ms(100);
    _checksrq(d);
}

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int d, c;
    char *function =    R5005_FUNC_DCV;
    char *range =       R5005_RANGE_AUTO;
    char *trigger =     R5005_TRIGGER_INT;
    char *null =        R5005_NULL_DISABLE;
    char *percent =     R5005_PCT_DISABLE;
    char *lah =         R5005_LAH_DISABLE;
    char *reference =   R5005_REF_INTERNAL;
    char *time =        R5005_TIME_DISABLE;
    char *highres =     R5005_HIGHRES_OFF;
    char *srqmode =     R5005_SRQ_NONE;
    int samples = 1;
    long period_ms = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
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

    /* find device in /etc/gpib.conf */
    d = ibfind(instrument);
    if (d < 0) {
        fprintf(stderr, "%s: not found: %s\n", prog, instrument);
        exit(1);
    }

    /* clear dmm state */
    _ibclr(d);
    _ibwrtf(d, "%s", R5005_CMD_INIT);

    /* configure dmm */
    _ibwrtf(d, "%s%s%s%s%s%s%s%s%s%s", 
            function, range, trigger, null, percent, lah, reference, 
                time, highres, srqmode);

    /* take some readings and send to stdout */
    if (samples > 0) {
        char buf[1024];
        long t0, t1, t2;
        int ready;

        t0 = 0;
        while (samples-- > 0) {
            t1 = _gettime_ms();
            if (t0 == 0)
                t0 = t1;
 
            _ibwrtf(d, "%s", R5005_SRQ_READY); 

            if (period_ms > 0 && (!strcmp(trigger, R5005_TRIGGER_HOLD)
                              || !strcmp(trigger, R5005_TRIGGER_HOLD_DELAY)))
                _ibtrg(d);

            /* Block until SRQ and serial poll indicates data available.
             */
            ready = 0;
            while (!ready) {
                ibwait(BOARD, SRQI);
                assert(ibsta & SRQI);
                _spoll(d, &ready);
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
        _ibwrtf(d, "%s", R5005_SRQ_NONE); 
    }

    /* give back the front panel */
    _ibloc(d); 
    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
