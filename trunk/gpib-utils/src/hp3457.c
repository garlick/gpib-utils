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

#include "hp3457.h"
#include "units.h"

#define INSTRUMENT "hp3457" /* the /etc/gpib.conf entry */
#define BOARD       0       /* minor board number in /etc/gpib.conf */

static char *prog = "";
static int verbose = 0;

#define OPTIONS "n:f:r:t:m:H:s:T:d:vclaS"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument, 0, 'v'},
    {"function",        required_argument, 0, 'f'},
    {"range",           required_argument, 0, 'r'},
    {"trigger",         required_argument, 0, 't'},
    {"autocal",         no_argument,       0, 'a'},
    {"selftest",        no_argument,       0, 'S'},
    {"highres",         required_argument, 0, 'H'},
    {"samples",         required_argument, 0, 's'},
    {"period",          required_argument, 0, 'T'},
    {"digits",          required_argument, 0, 'd'},
    {0, 0, 0, 0},
};

static void _ibrdstr(int d, char *buf, int len, int checksrq);
static void _ibwrtstr(int d, char *str, int checksrq);

static void 
usage(void)
{
    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name                     dmm name [%s]\n"
"  -c,--clear                         clear instrument, set remote defaults\n"
"  -l,--local                         enable front panel, set local defaults\n"
"  -v,--verbose                       show protocol on stderr\n"
"  -a,--autocal                       autocalibrate\n"
"  -S,--selftest                      run selftest\n"
"  -f,--function dcv|acv|acdcv|dci|aci|acdci|ohm2|ohm4|freq|period\n"
"                                     select function [dcv]\n"
"  -r,--range <maxval>|auto           select range [auto]\n"
"  -t,--trigger auto|ext|sgl|hold|syn select trigger mode [syn]\n"
"  -d,--digits 3|4|5|6                number of digits [5]\n"
"  -s,--samples                       number of samples [0]\n"
"  -T,--period                        sample period in s[0]\n"
           , prog, INSTRUMENT);
    exit(1);
}

static double _gettime(void)
{
    struct timeval t;

    if (gettimeofday(&t, NULL) < 0) {
        perror("gettimeofday");
        exit(1);
    }
    return ((double)t.tv_sec + (double)t.tv_usec / 1000000);
}

static void _sleep_sec(double sec)
{
    if (sec > 0)
        usleep((unsigned long)(sec * 1000000.0));
}

static unsigned long _checkauxerr(int d)
{
    char tmpbuf[64];
    unsigned long err;

    _ibwrtstr(d, HP3457_AUXERR_QUERY, 0);
    _ibrdstr(d, tmpbuf, sizeof(tmpbuf), 0);

    err = strtoul(tmpbuf, NULL, 10);

    if (err & HP3457_AUXERR_OISO)
        fprintf(stderr, "%s: error: isolation error during operation\n", prog);
    if (err & HP3457_AUXERR_SLAVE)
        fprintf(stderr, "%s: error: slave processor self-test failure\n",prog);
    if (err & HP3457_AUXERR_TISO)
        fprintf(stderr, "%s: error: isolation self-test failure\n", prog);
    if (err & HP3457_AUXERR_ICONV)
        fprintf(stderr, "%s: error: integrator convergence failure\n", prog);
    if (err & HP3457_AUXERR_ZERO)
        fprintf(stderr, "%s: error: front end zero measurement\n", prog);
    if (err & HP3457_AUXERR_IGAINDIV)
        fprintf(stderr, "%s: error: curent src, gain sel, div fail\n", prog);
    if (err & HP3457_AUXERR_AMPS)
        fprintf(stderr, "%s: error: amps self-test failure\n", prog);
    if (err & HP3457_AUXERR_ACAMP)
        fprintf(stderr, "%s: error: ac amp dc offset failure\n", prog);
    if (err & HP3457_AUXERR_ACFLAT)
        fprintf(stderr, "%s: error: ac flatness check\n", prog);
    if (err & HP3457_AUXERR_OHMPC)
        fprintf(stderr, "%s: error: ohms precharge fail in autocal\n", prog);
    if (err & HP3457_AUXERR_32KROM)
        fprintf(stderr, "%s: error: 32K ROM checksum failure\n", prog);
    if (err & HP3457_AUXERR_8KROM)
        fprintf(stderr, "%s: error: 8K ROM checksum failure\n", prog);
    if (err & HP3457_AUXERR_NVRAM)
        fprintf(stderr, "%s: error: NVRAM failure\n", prog);
    if (err & HP3457_AUXERR_RAM)
        fprintf(stderr, "%s: error: volatile RAM failure\n", prog);
    if (err & HP3457_AUXERR_CALRAM)
        fprintf(stderr, "%s: error: cal RAM protection failure\n", prog);
    if (err & HP3457_AUXERR_CALRAM)
        fprintf(stderr, "%s: error: cal RAM protection failure\n", prog);

    return err;
}

static unsigned long _checkerr(int d)
{
    char tmpbuf[64];
    unsigned long err;

    _ibwrtstr(d, HP3457_ERR_QUERY, 0);
    _ibrdstr(d, tmpbuf, sizeof(tmpbuf), 0);

    err = strtoul(tmpbuf, NULL, 10);

    if (err & HP3457_ERR_HARDWARE) {
        fprintf(stderr, "%s: error: hardware (consult aux err reg)\n", prog);
        _checkauxerr(d);
    }
    if (err & HP3457_ERR_CAL_ACAL)
        fprintf(stderr, "%s: error: CAL or ACAL process\n", prog);
    if (err & HP3457_ERR_TOOFAST)
        fprintf(stderr, "%s: error: trigger too fast\n", prog);
    if (err & HP3457_ERR_SYNTAX)
        fprintf(stderr, "%s: error: syntax\n", prog);
    if (err & HP3457_ERR_COMMAND)
        fprintf(stderr, "%s: error: unknown command received\n", prog);
    if (err & HP3457_ERR_PARAMETER)
        fprintf(stderr, "%s: error: unknown parameter received\n", prog);
    if (err & HP3457_ERR_RANGE)
        fprintf(stderr, "%s: error: parameter out of range\n", prog);
    if (err & HP3457_ERR_MISSING)
        fprintf(stderr, "%s: error: required parameter missing\n", prog);
    if (err & HP3457_ERR_IGNORED)
        fprintf(stderr, "%s: error: parameter ignored\n", prog);
    if (err & HP3457_ERR_CALIBRATION)
        fprintf(stderr, "%s: error: out of calibration\n", prog);
    if (err & HP3457_ERR_AUTOCAL)
        fprintf(stderr, "%s: error: autocal required\n", prog);

    return err;
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
    if (status & HP3457_STAT_READY) {
        rdy = 1;
    }
    if (status & HP3457_STAT_SRQ_BUTTON) {
        fprintf(stderr, "%s: srq: front panel SRQ key pressed\n", prog);
        _ibwrtstr(d, HP3457_CSB, 0);
    }
    if (status & HP3457_STAT_SRQ_POWER) {
        fprintf(stderr, "%s: srq: power-on SRQ occurred\n", prog);
        _ibwrtstr(d, HP3457_CSB, 0);
    }
    if (status & HP3457_STAT_HILO_LIMIT) {
        fprintf(stderr, "%s: srq: hi or lo limit exceeded\n", prog);
        _ibwrtstr(d, HP3457_CSB, 0);
        exit(1);
    }
    if (status & HP3457_STAT_ERROR) {
        fprintf(stderr, "%s: srq: error (consult error register)\n", prog);
        _checkerr(d);
        _ibwrtstr(d, HP3457_CSB, 0);
        exit(1);
    }
    if (rdyp)
        *rdyp = rdy;
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
    sleep(1); /* found this by trial and error */
    _checksrq(d, NULL);
}

static void _ibwrtstr(int d, char *str, int checksrq)
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
    if (checksrq) /* so we can call from _checksrq */
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
        if (n == -1) {
            fprintf(stderr, "%s: out of memory\n", prog);
            exit(1);
        }
        _ibwrtstr(d, s, 1);
        free(s);
}

static void _ibrdstr(int d, char *buf, int len, int checksrq)
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
    if (checksrq) /* so we can call from _checksrq */
        _checksrq(d, NULL);
    if (verbose)
        fprintf(stderr, "R: %s", buf);
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

static void _checkid(int d)
{
    char tmpbuf[64];

    _ibwrtf(d, HP3457_ID_QUERY);
    _ibrdstr(d, tmpbuf, sizeof(tmpbuf), 1);
    if (strncmp(tmpbuf, HP3457_ID_RESPONSE, strlen(HP3457_ID_RESPONSE)) != 0) {
        fprintf(stderr, "%s: device id %s != %s\n", prog, tmpbuf,
                HP3457_ID_RESPONSE);
        exit(1);
    }
}

static void _checkrange(int d)
{
    char tmpbuf[64];

    _ibwrtf(d, "%s", HP3457_RANGE_QUERY);
    _ibrdstr(d, tmpbuf, sizeof(tmpbuf), 1);
    fprintf(stderr, "%s: range is %.1lf\n", prog, strtod(tmpbuf, NULL));
}

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int d, c;
    char *funstr = NULL;
    char *rangestr = NULL;
    char *trigstr = NULL;
    char *digstr = NULL;
    double range = -1; /* auto */
    int samples = 0;
    double period = 0;
    double freq;
    int clear = 0;
    int local = 0;
    int autocal = 0;
    int selftest = 0;
    int showtime = 0;

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
        case 'a': /* --autocal */
            autocal = 1;
            break;
        case 'S': /* --selftest */
            selftest = 1;
            break;
        case 'v': /* --verbose */
            verbose = 1;
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
        case 't': /* --trigger */
            if (strcasecmp(optarg, "auto") == 0)
                trigstr = HP3457_TRIG_AUTO;
            else if (strcasecmp(optarg, "ext") == 0)
                trigstr = HP3457_TRIG_EXT;
            else if (strcasecmp(optarg, "sgl") == 0)
                trigstr = HP3457_TRIG_SGL;
            else if (strcasecmp(optarg, "hold") == 0)
                trigstr = HP3457_TRIG_HOLD;
            else if (strcasecmp(optarg, "syn") == 0)
                trigstr = HP3457_TRIG_SYN;
            else
                usage();
            break;
        case 'r': /* --range */
            rangestr = HP3457_RANGE;
            if (!strcasecmp(optarg, "auto"))
                range = -1; /* signifies auto */
            else
                range = strtod(optarg, NULL);
            break;
        case 'd': /* --digits */
            if (*optarg == '3')
                digstr = HP3457_NDIG3;
            else if (*optarg == '4')
                digstr = HP3457_NDIG4;
            else if (*optarg == '5')
                digstr = HP3457_NDIG5;
            else if (*optarg == '6')
                digstr = HP3457_NDIG6;
            else
                usage();
            break;
        case 'f': /* --function */
            if (strcasecmp(optarg, "dcv") == 0)
                funstr = HP3457_DCV;
            else if (strcasecmp(optarg, "acv") == 0)
                funstr = HP3457_ACV;
            else if (strcasecmp(optarg, "acdcv") == 0)
                funstr = HP3457_ACDCV;
            else if (strcasecmp(optarg, "dci") == 0)
                funstr = HP3457_DCI;
            else if (strcasecmp(optarg, "aci") == 0)
                funstr = HP3457_ACI;
            else if (strcasecmp(optarg, "acdci") == 0)
                funstr = HP3457_ACDCI;
            else if (strcasecmp(optarg, "freq") == 0)
                funstr = HP3457_FREQ;
            else if (strcasecmp(optarg, "period") == 0)
                funstr = HP3457_PERIOD;
            else if (strcasecmp(optarg, "ohm2") == 0)
                funstr = HP3457_2WIRE_OHMS;
            else if (strcasecmp(optarg, "ohm4") == 0)
                funstr = HP3457_4WIRE_OHMS;
            else
                usage();
            break;
        default:
            usage();
            break;
        }
    }

    if (!clear && !local && !autocal && !selftest 
            && !funstr && !rangestr && !trigstr && !digstr && samples == 0)
        usage();

    /* find device in /etc/gpib.conf */
    d = ibfind(instrument);
    if (d < 0) {
        fprintf(stderr, "%s: not found: %s\n", prog, instrument);
        exit(1);
    }

    /* Clear dmm state, verify that model is HP3457A, and
     * select remote defaults.
     */
    if (clear) {
        _ibclr(d);
        _checkid(d);
        _ibwrtf(d, HP3457_PRESET);
    }

    /* Perform autocalibration.
     */
    if (autocal) {
        _ibwrtf(d, HP3457_ACAL_BOTH);
        fprintf(stderr, "%s: waiting 35 sec for AC+ohms autocal\n", prog);
        fprintf(stderr, "%s: inputs should be disconnected\n", prog);
        sleep(35); 
        _checksrq(d, NULL); /* report any errors */
        fprintf(stderr, "%s: autocal complete\n", prog);
    }

    /* Perform selftest.
     */
    if (selftest) {
        _ibwrtf(d, HP3457_TEST);
        fprintf(stderr, "%s: waiting 7 sec for self-test\n", prog);
        sleep(7); 
        _checksrq(d, NULL); /* report any errors */
        fprintf(stderr, "%s: self-test complete\n", prog);
    }

    /* Select function.
     */
    if (funstr) {
        _ibwrtf(d, funstr);
    }

    /* Select range.
     */
    if (rangestr) {
        _ibwrtf(d, "%s %.1lf", rangestr, range);
        _checkrange(d);
    }

    /* Select trigger mode.
     */
    if (trigstr) {
        _ibwrtf(d, trigstr);
    }

    /* Select display digits.
     */
    if (digstr) {
        _ibwrtf(d, digstr);
    }

    if (samples > 0) {
        char buf[1024];
        double t0, t1, t2;

        if (digstr || trigstr || rangestr || funstr)
            sleep(1);

        t0 = 0;
        while (samples-- > 0) {
            t1 = _gettime();
            if (t0 == 0)
                t0 = t1;
            _ibrdstr(d, buf, sizeof(buf), 1);
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

    /* give back the front panel and set defaults for local operation */
    if (local) {
        _ibwrtf(d, HP3457_RESET);
        _ibloc(d); 
    }

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
