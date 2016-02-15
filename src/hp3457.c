/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2005-2009 Jim Garlick <garlick.jim@gmail.com>

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
#include <stdint.h>

#include "libutil/util.h"
#include "gpib.h"

#define INSTRUMENT "hp3457"

/* Status byte bits.
 */
#define HP3457_STAT_EXEC_DONE  0x01        /* program memory exec completed */
#define HP3457_STAT_HILO_LIMIT 0x02        /* hi or lo limited exceeded */
#define HP3457_STAT_SRQ_BUTTON 0x04        /* front panel SRQ key pressed */
#define HP3457_STAT_SRQ_POWER  0x08        /* power-on SRQ occurred */
#define HP3457_STAT_READY      0x10        /* ready */
#define HP3457_STAT_ERROR      0x20        /* error (consult error reg) */
#define HP3457_STAT_SRQ        0x40        /* service requested */

/* Error register bits.
 */
#define HP3457_ERR_HARDWARE    0x0001      /* hw error (consult aux error reg) */
#define HP3457_ERR_CAL_ACAL    0x0002      /* error in CAL or ACAL process */
#define HP3457_ERR_TOOFAST     0x0004      /* trigger too fast */
#define HP3457_ERR_SYNTAX      0x0008      /* syntax error */
#define HP3457_ERR_COMMAND     0x0010      /* unknown command received */
#define HP3457_ERR_PARAMETER   0x0020      /* unknown parameter received */
#define HP3457_ERR_RANGE       0x0040      /* parameter out of range */
#define HP3457_ERR_MISSING     0x0080      /* required parameter missing */
#define HP3457_ERR_IGNORED     0x0100      /* parameter ignored */
#define HP3457_ERR_CALIBRATION 0x0200      /* out of calibration */
#define HP3457_ERR_AUTOCAL     0x0400      /* autocal required */

/* Aux error register bits.
 */
#define HP3457_AUXERR_OISO     0x0001      /* isolation error during operation */
                                           /*   in any mode (self-test, */
                                           /*   autocal, measurements, etc) */
#define HP3457_AUXERR_SLAVE    0x0002      /* slave processor self-test failure */
#define HP3457_AUXERR_TISO     0x0004      /* isolation self-test failure */
#define HP3457_AUXERR_ICONV    0x0008      /* integrator convergence error */
#define HP3457_AUXERR_ZERO     0x0010      /* front end zero meas error */
#define HP3457_AUXERR_IGAINDIV 0x0020      /* current src, gain sel, div fail */
#define HP3457_AUXERR_AMPS     0x0040      /* amps self-test failure */
#define HP3457_AUXERR_ACAMP    0x0080      /* ac amp dc offset failure */
#define HP3457_AUXERR_ACFLAT   0x0100      /* ac flatness check */
#define HP3457_AUXERR_OHMPC    0x0200      /* ohms precharge fail in autocal */
#define HP3457_AUXERR_32KROM   0x0400      /* 32K ROM checksum failure */
#define HP3457_AUXERR_8KROM    0x0800      /* 8K ROM checksum failure */
#define HP3457_AUXERR_NVRAM    0x1000      /* NVRAM failure */
#define HP3457_AUXERR_RAM      0x2000      /* volatile RAM failure */
#define HP3457_AUXERR_CALRAM   0x4000      /* cal RAM protection failure */

static void _checkid(gd_t gd);
static void _checkrange(gd_t gd);
static double _gettime(void);
static void _sleep_sec(double sec);
static int _interpret_status(gd_t gd, unsigned char status, char *msg);
static void _checkid(gd_t gd);
static void _checkrange(gd_t gd);

char *prog = "";

static const char *options = OPTIONS_COMMON "lf:r:t:m:H:s:T:d:cAS";

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    OPTIONS_COMMON_LONG,
    {"local",           no_argument,       0, 'l'},
    {"clear",           no_argument,       0, 'c'},
    {"function",        required_argument, 0, 'f'},
    {"range",           required_argument, 0, 'r'},
    {"trigger",         required_argument, 0, 't'},
    {"autocal",         no_argument,       0, 'A'},
    {"selftest",        no_argument,       0, 'S'},
    {"highres",         required_argument, 0, 'H'},
    {"samples",         required_argument, 0, 's'},
    {"period",          required_argument, 0, 'T'},
    {"digits",          required_argument, 0, 'd'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static opt_desc_t optdesc[] = {
    OPTIONS_COMMON_DESC,
    { "l", "local", "return instrument to front panel control" },
    { "c", "clear", "clear instrument, set remote defaults" },
    { "A", "autocal", "autocalibrate" },
    { "S", "selftest", "run selftest" },
    { "f", "function", "select function [dcv]" },
    { "r", "range", "select range [auto]" },
    { "t", "trigger", "select trigger mode [syn]" },
    { "d", "digits", "number of digits [5]" },
    { "s", "samples", "number of samples [0]" },
    { "T", "period", "sample period [0]" },
    { 0, 0, 0 },
};

int
main(int argc, char *argv[])
{
    gd_t gd;
    double freq = 0, period = 0;
    int c, samples = 0;
    int print_usage = 0, showtime = 0;

    gd = gpib_init_args(argc, argv, options, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage)
        usage(optdesc);
    if (!gd)
        exit(1);

    gpib_set_reos(gd, 1);

    /*
     * Handle options.
     */
    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            OPTIONS_COMMON_SWITCH;
                break;
            case 'l': /* --local */
                gpib_loc(gd);
                break;
            case 'c': /* --clear */
                gpib_clr(gd, 1000000);
                _checkid(gd);
                gpib_wrtf(gd, "PRESET");
                break;
            case 'A': /* --autocal */
                gpib_wrtf(gd, "ACAL ALL");
                fprintf(stderr, "%s: waiting 35s for AC+ohms autocal\n", prog);
                fprintf(stderr, "%s: inputs should be disconnected\n", prog);
                sleep(35); 
                fprintf(stderr, "%s: autocal complete\n", prog);
                break;
            case 'S': /* --selftest */
                gpib_wrtf(gd, "TEST");
                fprintf(stderr, "%s: waiting 7 sec for self-test\n", prog);
                sleep(7); 
                fprintf(stderr, "%s: self-test complete\n", prog);
                break;
            case 'T': /* --period */
                if (freqstr(optarg, &freq) < 0) {
                    fprintf(stderr, "%s: error parsing period arg\n", prog);
                    fprintf(stderr, "%s: use freq units: %s\n", 
                            prog, FREQ_UNITS);
                    fprintf(stderr, "%s: or period units: %s\n", 
                            prog, PERIOD_UNITS);
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
                gpib_wrtf(gd, "TRIG %s", optarg);
                break;
            case 'r': /* --range */
                if (!strcasecmp(optarg, "auto"))
                    gpib_wrtf(gd, "RANGE -1");
                else
                    gpib_wrtf(gd, "RANGE %s", optarg);
                _checkrange(gd);
                break;
            case 'd': /* --digits */
                gpib_wrtf(gd, "NDIG %s", optarg);
                break;
            case 'f': /* --function */
                gpib_wrtf(gd, "FUNC %s", optarg);
                break;
        }
    }

    if (samples > 0) {
        char buf[1024];
        double t0, t1, t2;

	sleep(1);

        t0 = 0;
        while (samples-- > 0) {
            t1 = _gettime();
            if (t0 == 0)
                t0 = t1;
            gpib_rdstr(gd, buf, sizeof(buf));
            t2 = _gettime();

            /* Output suitable for gnuplot:
             *   t(s)  sample 
             * sample already has a crlf terminator
             */
            if (showtime)
                printf("%-3.3lf\t%s\n", (t1-t0), buf); 
            else
                printf("%s\n", buf); 

            if (samples > 0 && period > 0)
                _sleep_sec(period - (t2-t1));
        }
    }

    gpib_fini(gd);

    exit(0);
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

static unsigned long 
_checkauxerr(gd_t gd)
{
    char tmpbuf[64];
    uint16_t err;

    gpib_wrtstr(gd, "AUXERR?");
    gpib_rdstr(gd, tmpbuf, sizeof(tmpbuf));

    err = (uint16_t)strtoul(tmpbuf, NULL, 10);

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

static unsigned long 
_checkerr(gd_t gd)
{
    char tmpbuf[64];
    uint16_t err;

    gpib_wrtstr(gd, "ERR?");
    gpib_rdstr(gd, tmpbuf, sizeof(tmpbuf));

    err = (uint16_t)strtoul(tmpbuf, NULL, 10);

    if (err & HP3457_ERR_HARDWARE) {
        fprintf(stderr, "%s: error: hardware (consult aux err reg)\n", prog);
        _checkauxerr(gd);
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

/* Interpet serial poll results (status byte)
 * Return: 0=non-fatal/no error, >0=fatal, -1=retry.
 */
static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

    if (status & HP3457_STAT_SRQ_BUTTON) {
        fprintf(stderr, "%s: srq: front panel SRQ key pressed\n", prog);
        gpib_wrtstr(gd, "CSB");
    }
    if (status & HP3457_STAT_SRQ_POWER) {
        fprintf(stderr, "%s: srq: power-on SRQ occurred\n", prog);
        gpib_wrtstr(gd, "CSB");
    }
    if (status & HP3457_STAT_HILO_LIMIT) {
        fprintf(stderr, "%s: srq: hi or lo limit exceeded\n", prog);
        gpib_wrtstr(gd, "CSB");
        err = 1;
    }
    if (status & HP3457_STAT_ERROR) {
        fprintf(stderr, "%s: srq: error (consult error register)\n", prog);
        _checkerr(gd);
        gpib_wrtstr(gd, "CSB");
        err = 1;
    }
    if (status & HP3457_STAT_READY) {
        /* do nothing */
    }
    return err;
}

static void 
_checkid(gd_t gd)
{
    char tmpbuf[64];

    gpib_wrtf(gd, "ID?");
    gpib_rdstr(gd, tmpbuf, sizeof(tmpbuf));
    if (strncmp(tmpbuf, "HP3457A", 7) != 0) {
        fprintf(stderr, "%s: device id %s != %s\n", prog, tmpbuf, "HP3457A");
        exit(1);
    }
}

static void 
_checkrange(gd_t gd)
{
    char tmpbuf[64];

    gpib_wrtf(gd, "RANGE?");
    gpib_rdstr(gd, tmpbuf, sizeof(tmpbuf));
    /* FIXME: more digits need to be displayed here (how many - chk manual) */
    fprintf(stderr, "%s: range is %.1lf\n", prog, strtod(tmpbuf, NULL));
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
