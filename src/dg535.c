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

char *prog = "";
static int verbose = 0;

#define OPTIONS "n:clvo:m:a:O:p:T:t:M:D:"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"output-chan",     required_argument, 0, 'o'},
    {"output-mode",     required_argument, 0, 'm'},
    {"output-ampl",     required_argument, 0, 'a'},
    {"output-offset",   required_argument, 0, 'O'},
    {"output-polarity", required_argument, 0, 'p'},
    {"output-z",        required_argument, 0, 'T'},
    {"trigger-rate",    required_argument, 0, 't'},
    {"trigger-mode",    required_argument, 0, 'M'},
    {"display-string",  required_argument, 0, 'D'},
    {0, 0, 0, 0},
};

static void 
usage(void)
{
    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name              instrument name [%s]\n"
"  -c,--clear                  initialize instrument to default values\n"
"  -l,--local                  return instrument to local operation on exit\n"
"  -v,--verbose                show protocol on stderr\n"
"  -o,--output-chan            output channel (t0|a|b|ab|c|d|cd)\n"
"  -m,--output-mode            output mode (ttl|nim|ecl|var)\n"
"  -a,--output-ampl            output amplitude (-4:-0.1, +0.1:+4) in volts\n"
"  -O,--output-offset          output offset (-4:+4) in volts\n"
"  -p,--output-polarity        output polarity (+|-)\n"
"  -T,--output-z               output impedence (hi|lo)\n"
"  -d,--delay                  output delay (chan,secs)\n"
"  -M,--trigger-mode           trigger mode (int|ext|ss|burst)\n"
"  -t,--trigger-rate           trigger rate (0.001hz:1.000mhz)\n"
"  -s,--trigger-slope          trigger slope (up|down)\n"
"  -b,--trigger-count          trigger burst count (2:32766)\n"
"  -z,--trigger-z              trigger input impedence (hi|lo)\n"
"  -D,--display-string         display string (1-20 chars), empty to clear\n"
           , prog, INSTRUMENT);
    exit(1);
}

static void
_check_err(gd_t gd)
{
    char buf[16];
    int status;

    /* check error status */
    gpib_wrtf(gd, "%s", DG535_ERROR_STATUS);
    gpib_rdstr(gd, buf, sizeof(buf));
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

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

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
        _check_err(gd);

    if (status != 0)
        err = 1;
    return err;
}

int
_outchan2int(char *str)
{
    if (!strcasecmp(str, "t0"))
        return DG535_DO_T0;
    else if (!strcasecmp(str, "a"))
        return DG535_DO_A;
    else if (!strcasecmp(str, "b"))
        return DG535_DO_B;
    else if (!strcasecmp(str, "ab"))
        return DG535_DO_AB;
    else if (!strcasecmp(str, "c"))
        return DG535_DO_C;
    else if (!strcasecmp(str, "d"))
        return DG535_DO_D;
    else if (!strcasecmp(str, "cd"))
        return DG535_DO_CD;
    return -1;
}

int
_outmode2int(char *str)
{
    if (!strcasecmp(str, "ttl"))
        return 0;
    else if (!strcasecmp(str, "nim"))
        return 1;
    else if (!strcasecmp(str, "ecl"))
        return 2;
    else if (!strcasecmp(str, "var"))
        return 3;
    return -1;
}


int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int c;
    int clear = 0;
    int local = 0;
    double trigfreq = 0.0;
    gd_t gd;
    char *string = NULL;
    int out_chan = -1;
    int out_mode = -1;
    double out_ampl = HUGE_VALF;
    double out_offset = HUGE_VALF;
    int out_polarity = -1;
    int out_z = -1;

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
        case 'D': /* --display-string */
            string = optarg;
            break;
        case 'o': /* --output-chan */
            out_chan = _outchan2int(optarg);
            if (out_chan == -1)
                usage();
            break;
        case 'm': /* --output-mode */
            out_mode = _outmode2int(optarg);
            if (out_mode == -1)
                usage();
            break;
        case 'a': /* --output-ampl */
            out_ampl = strtod(optarg, NULL);
            break;
        case 'O': /* --output-offset */
            out_offset = strtod(optarg, NULL);
            break;
        case 'p': /* --output-polarity */
            if (!strcasecmp(optarg, "-"))
                out_polarity = 0;
            else if (!strcasecmp(optarg, "+"))
                out_polarity = 1;
            else
                usage();
            break;
        case 'T': /* --output-z */
            if (!strcasecmp(optarg, "lo"))
                out_z = 0;
            else if (!strcasecmp(optarg, "hi"))
                out_z = 1;
            else
                usage();
            break;
        case 't': /* --triger-rate */
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
    if (!clear && !local && trigfreq == 0.0 && !string && out_mode == -1 
            && out_ampl == HUGE_VALF && out_offset == HUGE_VALF
            && out_polarity == -1 && out_z == -1) {
        usage();
    }
    if (out_chan == -1 && out_mode != -1) {
        fprintf(stderr, "%s: --output-mode requires --output-chan\n", prog);
        exit(1);
    }
    if (out_chan == -1 && out_ampl != HUGE_VALF) {
        fprintf(stderr, "%s: --output-ampl requires --output-chan\n", prog);
        exit(1);
    }
    if (out_chan == -1 && out_offset != HUGE_VALF) {
        fprintf(stderr, "%s: --output-offset requires --output-chan\n", prog);
        exit(1);
    }
    if (out_chan == -1 && out_polarity != -1) {
        fprintf(stderr, "%s: --output-polarity requires --output-chan\n", prog);
        exit(1);
    }
    if (out_chan == -1 && out_z != -1) {
        fprintf(stderr, "%s: --output-z requires --output-chan\n", prog);
        exit(1);
    }

    /* find device in /etc/gpib.conf */
    gd = gpib_init(instrument, _interpret_status, 0);
    if (!gd) {
        fprintf(stderr, "%s: couldn't find device %s in /etc/gpib.conf\n", 
                prog, instrument);
        exit(1);
    }
    gpib_set_verbose(gd, verbose);

    /* clear instrument to default settings */
    if (clear) {
        gpib_clr(gd, 0);
        gpib_wrtf(gd, "%s", DG535_CLEAR);
    }

    /* display string */
    if (string)
        gpib_wrtf(gd, "%s %s", DG535_DISPLAY_STRING, string);

    /* output configuration */
    if (out_mode != -1)
        gpib_wrtf(gd, "%s %d,%d", DG535_OUT_MODE, out_chan, out_mode);
    if (out_ampl != HUGE_VALF)
        gpib_wrtf(gd, "%s %d,%lf", DG535_OUT_AMPL, out_chan, out_ampl);
    if (out_offset != HUGE_VALF)
        gpib_wrtf(gd, "%s %d,%lf", DG535_OUT_OFFSET, out_chan, out_offset);
    if (out_polarity != -1)
        gpib_wrtf(gd, "%s %d,%d", DG535_OUT_POLARITY, out_chan, out_polarity);
    if (out_z != -1)
        gpib_wrtf(gd, "%s %d,%d", DG535_TERM_Z, out_chan, out_z);

    /* set internal trigger rate */
    if (trigfreq != 0.0) {
        gpib_wrtf(gd, "%s %d,%lf", DG535_TRIG_RATE, 0, trigfreq);
    }

    /* return front panel if requested */
    if (local)
        gpib_loc(gd); 

    gpib_fini(gd);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
