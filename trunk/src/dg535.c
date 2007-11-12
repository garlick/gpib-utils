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

/*#define INSTRUMENT "dg535"*/  /* default instrument name */
#define INSTRUMENT "gpibgw:gpib0,15"

char *prog = "";
static int verbose = 0;

#define OPTIONS "n:clvo:qQ:aA:fF:pP:yY:tT:mM:sS:bB:zZ:D:x"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"display-string",  required_argument, 0, 'D'},
    {"single-shot",     no_argument,       0, 'x'},
    {"out-chan",        required_argument, 0, 'o'},

    {"get-out-mode",    no_argument,       0, 'q'},
    {"set-out-mode",    required_argument, 0, 'Q'},

    {"get-out-ampl",    no_argument,       0, 'a'},
    {"set-out-ampl",    required_argument, 0, 'A'},

    {"get-out-offset",  no_argument,       0, 'f'},
    {"set-out-offset",  required_argument, 0, 'F'},

    {"get-out-polarity",no_argument,       0, 'p'},
    {"set-out-polarity",required_argument, 0, 'P'},

    {"get-out-z",       no_argument,       0, 'y'},
    {"set-out-z",       required_argument, 0, 'Y'},

    {"get-trig-rate",   no_argument,       0, 't'},
    {"set-trig-rate",   required_argument, 0, 'T'},

    {"get-trig-mode",   no_argument      , 0, 'm'},
    {"set-trig-mode",   required_argument, 0, 'M'},

    {"get-trig-slope",  no_argument,       0, 's'},
    {"set-trig-slope",  required_argument, 0, 'S'},

    {"get-burst-count", no_argument,       0, 'b'},
    {"set-burst-count", required_argument, 0, 'B'},

    {"get-trig-z",      no_argument,       0, 'z'},
    {"set-trig-z",      required_argument, 0, 'Z'},
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
"  -D,--display-string         display string (1-20 chars), empty to clear\n"
"  -x,--single-shot            trigger instrument in single shot mode\n"
"  -o,--out-chan               select output channel (t0|a|b|ab|c|d|cd)\n"
"  -q,--get-out-mode           get output mode\n"
"  -Q,--set-out-mode           set output mode (ttl|nim|ecl|var)\n"
"  -a,--get-out-ampl           get output amplitude\n"
"  -A,--set-out-ampl           set output amplitude (-4:-0.1, +0.1:+4) volts\n"
"  -f,--get-out-offset         get output offset\n"
"  -F,--set-out-offset         set output offset (-4:+4) volts\n"
"  -p,--get-out-polarity       get output polarity\n"
"  -P,--set-out-polarity       set output polarity (+|-)\n"
"  -y,--get-out-z              get output impedence\n"
"  -Y,--set-out-z              set output impedence (hi|lo)\n"
"  -m,--get-trig-mode          get trigger mode\n"
"  -M,--set-trig-mode          set trigger mode (int|ext|ss|bur)\n"
"  -t,--get-trig-rate          get trigger rate\n"
"  -T,--set-trig-rate          set trigger rate (0.001hz:1.000mhz)\n"
"  -s,--get-trig-slope         get trigger slope\n"
"  -S,--set-trig-slope         set trigger slope (rising|falling)\n"
"  -b,--get-burst-count        get trigger burst count\n"
"  -B,--set-burst-count        set trigger burst count (2:19)\n"
"  -z,--get-trig-z             get trigger input impedence\n"
"  -Z,--set-trig-z             set trigger input impedence (hi|lo)\n"

"  -d,--delay                  output delay (chan,secs)\n"
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
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

    if (status & DG535_STAT_BADMEM) {
        fprintf(stderr, "%s: memory contents corrupted\n", prog);
        err = 1;
    }
    if (status & DG535_STAT_SRQ) {
        fprintf(stderr, "%s: SRQ pending\n", prog);
        err = 1;
    }
    if (status & DG535_STAT_TOOFAST) {
        fprintf(stderr, "%s: trigger rate too high\n", prog);
        err = 1;
    }
    if (status & DG535_STAT_PLL) {
        fprintf(stderr, "%s: 80MHz PLL is unlocked\n", prog);
        err = 1;
    }
    /*if (status & DG535_STAT_TRIG)
        fprintf(stderr, "%s: trigger has occurred\n", prog);*/
    /*if (status & DG535_STAT_BUSY)
        fprintf(stderr, "%s: busy with timing cycle\n", prog);*/
    if (status & DG535_STAT_CMDERR) {
        _check_err(gd);
        err = 1;
    }
    return err;
}

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int c;
    int clear = 0;
    int local = 0;
    gd_t gd;
    char *display_string = NULL;
    int out_chan = -1;
    int get_out_mode = 0;
    int set_out_mode = -1;
    int get_out_ampl = 0;
    double set_out_ampl = HUGE_VALF;
    int get_out_offset = 0;
    double set_out_offset = HUGE_VALF;
    int get_out_polarity = 0;
    int set_out_polarity = -1;
    int get_out_z = 0;
    int set_out_z = -1;
    int get_trig_rate = 0;
    double set_trig_rate = 0.0;
    int get_trig_mode = 0;
    int set_trig_mode = -1;
    int get_burst_count = 0;
    int set_burst_count = -1;
    int get_trig_z = 0;
    int set_trig_z = -1;
    int get_trig_slope = 0;
    int set_trig_slope = -1;
    int exit_val = 0;
    int single_shot = 0;

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
            display_string = optarg;
            break;
        case 'x': /* --single-shot */
            single_shot = 1;
            break;
        case 'o': /* --out-chan */
            if (!strcasecmp(optarg, "t0"))
                out_chan = DG535_DO_T0;
            else if (!strcasecmp(optarg, "a"))
                out_chan = DG535_DO_A;
            else if (!strcasecmp(optarg, "b"))
                out_chan = DG535_DO_B;
            else if (!strcasecmp(optarg, "ab"))
                out_chan = DG535_DO_AB;
            else if (!strcasecmp(optarg, "c"))
                out_chan = DG535_DO_C;
            else if (!strcasecmp(optarg, "d"))
                out_chan = DG535_DO_D;
            else if (!strcasecmp(optarg, "cd"))
                out_chan = DG535_DO_CD;
            else {
                fprintf(stderr, "%s: bad --out-chan optarg\n", prog);
                exit(1);
            }
            break;
        case 'q' : /* --get-out-mode */
            get_out_mode = 1;
            break;
        case 'Q': /* --set-out-mode */
            if (!strcasecmp(optarg, "ttl"))
                set_out_mode = DG535_OUT_TTL;
            else if (!strcasecmp(optarg, "nim"))
                set_out_mode = DG535_OUT_NIM;
            else if (!strcasecmp(optarg, "ecl"))
                set_out_mode = DG535_OUT_ECL;
            else if (!strcasecmp(optarg, "var"))
                set_out_mode = DG535_OUT_VAR;
            else {
                fprintf(stderr, "%s: bad --out-mode optarg\n", prog);
                exit(1);
            }
            break;
        case 'a': /* --get-out-ampl */
            get_out_ampl = 1;
            break;
        case 'A': /* --set-out-ampl */
            set_out_ampl = strtod(optarg, NULL);
            break;
        case 'f' : /* --get-out-offset */
            get_out_offset = 1;
            break;
        case 'F': /* --set-out-offset */
            set_out_offset = strtod(optarg, NULL);
            break;
        case 'p': /* --get-out-polarity */
            get_out_polarity = 1;
            break;
        case 'P': /* --set-out-polarity */
            if (!strcasecmp(optarg, "-"))
                set_out_polarity = 0;
            else if (!strcasecmp(optarg, "+"))
                set_out_polarity = 1;
            else
                usage();
            break;
        case 'y': /* --get-out-z */
            get_out_z = 1;
            break;
        case 'Y': /* --set-out-z */
            if (!strcasecmp(optarg, "lo"))
                set_out_z = 0;
            else if (!strcasecmp(optarg, "hi"))
                set_out_z = 1;
            else
                usage();
            break;
        case 'm' : /* --get-trig-mode */
            get_trig_mode = 1;
            break;
        case 'M' : /* --set-trig-mode */
            if (!strcasecmp(optarg, "int"))
                set_trig_mode = DG535_TRIG_INT;
            else if (!strcasecmp(optarg, "ext"))
                set_trig_mode = DG535_TRIG_EXT;
            else if (!strcasecmp(optarg, "ss"))
                set_trig_mode = DG535_TRIG_SS;
            else if (!strcasecmp(optarg, "bur"))
                set_trig_mode = DG535_TRIG_BUR;
            else {
                fprintf(stderr, "%s: bad --trig-mode optarg\n", prog);
                exit(1);
            }
            break;
        case 't': /* --get-trig-rate */
            get_trig_rate = 1;
            break;
        case 'T': /* --set-trig-rate */
            if (freqstr(optarg, &set_trig_rate) < 0) {
                fprintf(stderr, "%s: error converting trigger freq\n", prog);
                fprintf(stderr, "%s: use freq units: %s\n", prog, FREQ_UNITS);
                fprintf(stderr, "%s: or period units: %s\n", prog,PERIOD_UNITS);
                exit(1);
            }
            break;
        case 's' : /* --get-trig-slope */
            get_trig_slope = 1;
            break;
        case 'S' : /* --set-trig-slope */
            if (!strcasecmp(optarg, "rising"))
                set_trig_slope = 1;
            else if (!strcasecmp(optarg, "falling"))
                set_trig_slope = 0;
            else
                usage();
            break;
        case 'b' : /* --get-trig-count */
            get_burst_count = 1;
            break;
        case 'B' : /* --set-trig-count */
            set_burst_count = strtoul(optarg, NULL, 0);
            break;
        case 'z' : /* --get-trig-z */
            get_trig_z = 1;
            break;
        case 'Z' : /* --set-trig-z */
            if (!strcasecmp(optarg, "lo"))
                set_trig_z = 0;
            else if (!strcasecmp(optarg, "hi"))
                set_trig_z = 1;
            else
                usage();
            break;
        default:
            usage();
            break;
        }
    }
    if (!clear && !local && !display_string 
            && !get_out_mode && set_out_mode == -1 
            && !get_out_ampl && set_out_ampl == HUGE_VALF 
            && !get_out_offset && set_out_offset == HUGE_VALF
            && !get_out_polarity && set_out_polarity == -1 
            && !get_out_z && set_out_z == -1 
            && !get_trig_mode && set_trig_mode == -1 
            && !get_trig_rate && set_trig_rate == 0.0 
            && !get_trig_z && set_trig_z == -1 
            && !get_burst_count && set_burst_count == -1 
            && !get_trig_slope && set_trig_slope == -1
            && !single_shot) {
        usage();
    }
    if (out_chan == -1 && (get_out_mode || set_out_mode != -1)) {
        fprintf(stderr, "%s: --get/set-out-mode needs --out-chan\n", prog);
        exit(1);
    }
    if (out_chan == -1 && (get_out_ampl || set_out_ampl != HUGE_VALF)) {
        fprintf(stderr, "%s: --get/set-out-ampl needs --out-chan\n", prog);
        exit(1);
    }
    if (out_chan == -1 && (get_out_offset || set_out_offset != HUGE_VALF)) {
        fprintf(stderr, "%s: --get/set-output-off needs --out-chan\n", prog);
        exit(1);
    }
    if (out_chan == -1 && (get_out_polarity || set_out_polarity != -1)) {
        fprintf(stderr, "%s: --get/set-out-polarity needs --out-chan\n", prog);
        exit(1);
    }
    if (out_chan == -1 && (get_out_z || set_out_z != -1)) {
        fprintf(stderr, "%s: --get/set-out-z needs --out-chan\n", prog);
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
    if (display_string)
        gpib_wrtf(gd, "%s %s", DG535_DISPLAY_STRING, display_string);

    /* outputs */
    if (set_out_mode != -1)
        gpib_wrtf(gd, "%s %d,%d", DG535_OUT_MODE, out_chan, set_out_mode);
    if (get_out_mode) {
        /* FIXME */
    }
    if (set_out_ampl != HUGE_VALF)
        gpib_wrtf(gd, "%s %d,%lf", DG535_OUT_AMPL, out_chan, set_out_ampl);
    if (get_out_ampl) {
        /* FIXME */
    }
    if (set_out_offset != HUGE_VALF)
        gpib_wrtf(gd, "%s %d,%lf", DG535_OUT_OFFSET, out_chan, set_out_offset);
    if (get_out_offset) {
        /* FIXME */
    }
    if (set_out_polarity != -1)
        gpib_wrtf(gd, "%s %d,%d", DG535_OUT_POLARITY, out_chan, set_out_polarity);
    if (get_out_polarity) {
        /* FIXME */
    }
    if (set_out_z != -1)
        gpib_wrtf(gd, "%s %d,%d", DG535_TERM_Z, out_chan, set_out_z);
    if (get_out_z) {
        /* FIXME */
    }

    /* trigger */
    if (set_trig_mode != -1)
        gpib_wrtf(gd, "%s %d", DG535_TRIG_MODE, set_trig_mode);
    if (get_trig_slope) {
        int tmp;

        gpib_wrtf(gd, "%s", DG535_TRIG_SLOPE);
        if (gpib_rdf(gd, "%d", &tmp) != 1) {
            fprintf(stderr, "%s: error reading trigger slope\n", prog);
            exit_val = 1;
            goto done;
        }
        fprintf(stderr, "%s: %s\n", prog, tmp == 0 ? "falling" : "rising");
    }
    if (get_burst_count) {
        int tmp;

        gpib_wrtf(gd, "%s", DG535_BURST_COUNT);
        if (gpib_rdf(gd, "%d", &tmp) != 1) {
            fprintf(stderr, "%s: error reading trigger slope\n", prog);
            exit_val = 1;
            goto done;
        }
        fprintf(stderr, "%s: %d\n", prog, tmp);
    }
    if (get_trig_z) {
        int tmp;

        gpib_wrtf(gd, "%s 0", DG535_TRIG_Z);
        if (gpib_rdf(gd, "%d", &tmp) != 1) {
            fprintf(stderr, "%s: error reading trigger impedence\n", prog);
            exit_val = 1;
            goto done;
        }
        fprintf(stderr, "%s: %s\n", prog, tmp == 0 ? "lo" : "hi");
    }
    if (set_trig_rate != 0.0 || set_trig_slope != -1 || set_burst_count != -1
            || get_trig_mode || set_trig_z != -1 || get_trig_rate
            || single_shot) {
        int tmode;

        gpib_wrtf(gd, "%s", DG535_TRIG_MODE);
        if (gpib_rdf(gd, "%d", &tmode) != 1) {
            fprintf(stderr, "%s: error reading trigger mode\n", prog);
            exit_val = 1;
            goto done;
        }
        if (tmode != DG535_TRIG_INT && tmode != DG535_TRIG_EXT
                && tmode != DG535_TRIG_SS && tmode != DG535_TRIG_BUR) {
            fprintf(stderr, "%s: read illegal trigger mode\n", prog);
            exit_val = 1;
            goto done;
        }
        if (get_trig_mode) {
            switch(tmode) {
                case DG535_TRIG_INT: 
                    fprintf(stderr, "%s: int\n", prog); 
                    break;
                case DG535_TRIG_EXT: 
                    fprintf(stderr, "%s: ext\n", prog); 
                    break;
                case DG535_TRIG_SS: 
                    fprintf(stderr, "%s: ss\n", prog); 
                    break;
                case DG535_TRIG_BUR: 
                    fprintf(stderr, "%s: bur\n", prog); 
                    break;
            }
        }
        if (set_trig_rate != 0.0 && tmode != DG535_TRIG_INT && tmode != DG535_TRIG_BUR) {
            fprintf(stderr, "%s: --set-trig-rate needs --set-trig-mode=int|bur\n", prog);
            exit_val = 1;
            goto done;
        }
        if (set_trig_slope != -1 && tmode != DG535_TRIG_EXT) {
            fprintf(stderr, "%s: --set-trig-slope needs --set-trig-mode=ext\n", prog);
            exit_val = 1;
            goto done;
        }
        if (set_burst_count != -1 && tmode != DG535_TRIG_BUR) {
            fprintf(stderr, "%s: --set-trig-count needs --set-trig-mode=bur\n", prog);
            exit_val = 1;
            goto done;
        }
        if (set_trig_z != -1 && tmode != DG535_TRIG_EXT) {
            fprintf(stderr, "%s: --set-trig-z needs --set-trig-mode=ext\n", prog);
            exit_val = 1;
            goto done;
        }
        if (single_shot && tmode != DG535_TRIG_SS) {
            fprintf(stderr, "%s: --single-shot needs --set-trig-mode=ss\n", prog);
            exit_val = 1;
            goto done;
        }
        if (set_trig_rate != 0.0 && tmode == DG535_TRIG_INT)
            gpib_wrtf(gd, "%s 0,%lf", DG535_TRIG_RATE, set_trig_rate);
        if (set_trig_rate != 0.0 && tmode == DG535_TRIG_BUR)
            gpib_wrtf(gd, "%s 1,%lf", DG535_TRIG_RATE, set_trig_rate);
        if (get_trig_rate) {
            double tmp;

            if (tmode == DG535_TRIG_INT)
                gpib_wrtf(gd, "%s 0", DG535_TRIG_RATE);
            else /* DG535_TRIG_BUR */
                gpib_wrtf(gd, "%s 1", DG535_TRIG_RATE);
            if (gpib_rdf(gd, "%lf", &tmp) != 1) {
                fprintf(stderr, "%s: error reading trigger rate\n", prog);
                exit_val = 1;
                goto done;
            }
            fprintf(stderr, "%s: %lfhz\n", prog, tmp);
        }
        if (set_trig_slope != -1)
            gpib_wrtf(gd, "%s %d", DG535_TRIG_SLOPE, set_trig_slope);
        if (set_burst_count != -1)
            gpib_wrtf(gd, "%s %d", DG535_BURST_COUNT, set_burst_count);
        if (set_trig_z != -1)
            gpib_wrtf(gd, "%s 0,%d", DG535_TRIG_Z, set_trig_z);
        if (single_shot)
            gpib_wrtf(gd, "%s", DG535_SINGLE_SHOT);
    }

    /* return front panel if requested */
    if (local)
        gpib_loc(gd); 

done:
    gpib_fini(gd);
    exit(exit_val);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
