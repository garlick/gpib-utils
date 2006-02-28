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
#include <stdint.h>

#include "dg535.h"
#include "units.h"
#include "gpib.h"

#define INSTRUMENT "dg535"  /* the /etc/gpib.conf entry */
#define BOARD       0       /* minor board number in /etc/gpib.conf */

static char *prog = "";
static int verbose = 0;

#define OPTIONS "n:clvt:"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"trigfreq",        required_argument, 0, 't'},
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
"  -t,--trigfreq                 set trigger freq (0.001hz - 1.000mhz)\n"
           , prog, INSTRUMENT);
    exit(1);
}

static void
_checkerr(int d)
{
    char buf[16];
    int status;

    /* check error status */
    gpib_ibwrtf(d, "%s", DG535_ERROR_STATUS);
    gpib_ibrdstr(d, buf, sizeof(buf));
    if (*buf < '0' || *buf > '9') {
        fprintf(stderr, "%s: error reading error status byte\n", prog);
        exit(1);
    }
    status = strtoul(buf, NULL, 10);
    if (status & DG535_ERR_RECALL) 
        fprintf(stderr, "%s: recalled data was corrupt\n", prog);
    if (status & DG535_ERR_DELAY_RANGE)
        fprintf(stderr, "%s: delay range error\n", prog);
    if (status & DG535_ERR_DELAY_LINKAGE)
        fprintf(stderr, "%s: delay linkage error\n", prog);
    if (status & DG535_ERR_CMDMODE)
        fprintf(stderr, "%s: wrong mode for command\n", prog);
    if (status & DG535_ERR_ERANGE)
        fprintf(stderr, "%s: value out of range\n", prog);
    if (status & DG535_ERR_NUMPARAM)
        fprintf(stderr, "%s: wrong number of parameters\n", prog);
    if (status & DG535_ERR_BADCMD)
        fprintf(stderr, "%s: unrecognized command\n", prog);
    if (status != 0)
        exit(1);
}

static void
_checkinst(int d)
{
    char buf[16];
    int status;

    /* check instrument status */
    gpib_ibwrtf(d, "%s", DG535_INSTR_STATUS);
    gpib_ibrdstr(d, buf, sizeof(buf));
    if (*buf < '0' || *buf > '9') {
        fprintf(stderr, "%s: error reading instrument status byte\n", prog);
        exit(1);
    }
    status = strtoul(buf, NULL, 10);

    if (status & DG535_STAT_BADMEM)
        fprintf(stderr, "%s: memory contents corrupted\n", prog);
    if (status & DG535_STAT_SRQ)
        fprintf(stderr, "%s: service request\n", prog);
    if (status & DG535_STAT_TOOFAST)
        fprintf(stderr, "%s: trigger rate too high\n", prog);
    if (status & DG535_STAT_PLL)
        fprintf(stderr, "%s: 80MHz PLL is unlocked\n", prog);
    if (status & DG535_STAT_TRIG)
        fprintf(stderr, "%s: trigger has occurred\n", prog);
    if (status & DG535_STAT_BUSY)
        fprintf(stderr, "%s: busy with timing cycle\n", prog);
    if (status & DG535_STAT_CMDERR)
        fprintf(stderr, "%s: command error detected\n", prog);

    if (status != 0)
        exit(1);
}

static void
dg535_checkerr(int d)
{
    _checkerr(d);
    _checkinst(d);
}

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int c, d;
    int clear = 0;
    int local = 0;
    double trigfreq = 0.0;

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
        case 't': /* --trigfreq */
            if (freqstr(optarg, &trigfreq) < 0) {
                fprintf(stderr, "%s: error converting trigger freq\n", prog);
                fprintf(stderr, "%s: use freq units: %s\n", prog, FREQ_UNITS);
                fprintf(stderr, "%s: or period units: %s\n", prog,PERIOD_UNITS);
                exit(1);
            }
            /* instrument checks range for us */
            break;
        default:
            usage();
            break;
        }
    }

    if (!clear && !local && trigfreq == 0.0)
        usage();

    /* find device in /etc/gpib.conf */
    d = gpib_init(prog, instrument, verbose);

    /* clear instrument to default settings */
    if (clear) {
        gpib_ibclr(d);
        gpib_ibwrtf(d, "%s", DG535_CLEAR);
        dg535_checkerr(d);
    }

    /* set internal trigger rate */
    if (trigfreq != 0.0) {
        gpib_ibwrtf(d, "%s %d,%lf", DG535_TRIG_RATE, 0, trigfreq);
        dg535_checkerr(d);
    }

    /* return front panel if requested */
    if (local)
        gpib_ibloc(d); 

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
