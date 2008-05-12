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

#if HAVE_CONFIG_H
#include "config.h"
#endif
#define _GNU_SOURCE /* for HUGE_VALF */
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
#include "gpib.h"
#include "util.h"

#define INSTRUMENT "dg535"

char *prog = "";
static int verbose = 0;
static strtab_t out_names[] = DG535_OUT_NAMES;
static strtab_t out_modes[] = DG535_OUT_MODES;
static strtab_t trig_modes[] = DG535_TRIG_MODES;

#define OPTIONS "a:clve:o:dD:qQ:gG:fF:pP:yY:tT:mM:sS:bB:zZ:D:x"
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"display-string",  required_argument, 0, 'e'},
    {"single-shot",     no_argument,       0, 'x'},
    {"out-chan",        required_argument, 0, 'o'},
    {"get-delay",       no_argument,       0, 'd'},
    {"set-delay",       required_argument, 0, 'D'},
    {"get-out-mode",    no_argument,       0, 'q'},
    {"set-out-mode",    required_argument, 0, 'Q'},
    {"get-out-ampl",    no_argument,       0, 'g'},
    {"set-out-ampl",    required_argument, 0, 'G'},
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
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -a,--address                instrument address [%s]\n"
"  -c,--clear                  initialize instrument to default values\n"
"  -l,--local                  return instrument to local operation on exit\n"
"  -v,--verbose                show protocol on stderr\n"
"  -e,--display-string         display string (1-20 chars), empty to clear\n"
"  -x,--single-shot            trigger instrument in single shot mode\n"
"  -o,--out-chan               select output channel (t0|a|b|ab|c|d|cd)\n"
"  -dD,--get/set-delay         output delay (chan,delay)\n"
"  -qQ,--get/set-out-mode      output mode (ttl|nim|ecl|var)\n"
"  -gG,--get/set-out-ampl      output amplitude (-4:-0.1, +0.1:+4) volts\n"
"  -fF,--get/set-out-offset    output offset (-4:+4) volts\n"
"  -pP,--get/set-out-polarity  output polarity (+|-)\n"
"  -yY,--get/set-out-z         output impedence (hi|lo)\n"
"  -mM,--get/set-trig-mode     trigger mode (int|ext|ss|bur)\n"
"  -tT,--get/set-trig-rate     trigger rate (0.001hz:1.000mhz)\n"
"  -sS,--get/set-trig-slope    trigger slope (rising|falling)\n"
"  -bB,--get/set-burst-count   trigger burst count (2:19)\n"
"  -zZ,--get/set-trig-z        trigger input impedence (hi|lo)\n"
           , prog, addr ? addr : "no default");
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

static int
_parse_delay(char *str, int *chanp, double *delayp)
{
    double f;
    char *cstr, *dstr;
    int c;

    if (!(cstr = strtok(str, ",")))
        return 0;
    if (!(dstr = strtok(NULL, ",")))
        return 0;
    if ((c = rfindstr(out_names, cstr)) == -1)
        return 0;
    *chanp = c;

    if (freqstr(dstr, &f) == -1) {
        fprintf(stderr, "%s: specify time units in s, ms, us, ns, ps\n", prog);
        return 0;
    }
    *delayp = 1.0/f;

    return 1;
}

int
main(int argc, char *argv[])
{
    char *addr = NULL;
    int c;
    int clear = 0;
    int local = 0;
    gd_t gd;
    char *display_string = NULL;
    int out_chan = -1;
    int get_delay = 0;
    double set_delay_time = HUGE_VALF;
    int set_delay_chan = -1;
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
    int todo = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'a': /* --address */
            addr = optarg;
            break;
        case 'v': /* --verbose */
            verbose = 1;
            break;
        case 'c': /* --clear */
            clear = 1;
            todo++;
            break;
        case 'l': /* --local */
            local = 1;
            todo++;
            break;
        case 'e': /* --display-string */
            display_string = optarg;
            todo++;
            break;
        case 'x': /* --single-shot */
            single_shot = 1;
            todo++;
            break;
        case 'o': /* --out-chan */
            out_chan = rfindstr(out_names, optarg);
            if (out_chan == -1) {
                fprintf(stderr, "%s: bad --out-chan argument\n", prog);
                exit(1);
            }
            break;
        case 'd': /* --get-delay */
            get_delay = 1;
            todo++;
            break;
        case 'D': /* --set-delay */
            if (!_parse_delay(optarg, &set_delay_chan, &set_delay_time)) {
                fprintf(stderr, "%s: bad --set-delay argument\n", prog);
                exit(1);
            }
            todo++;
            break;
        case 'q' : /* --get-out-mode */
            get_out_mode = 1;
            todo++;
            break;
        case 'Q': /* --set-out-mode */
            set_out_mode = rfindstr(out_modes, optarg);
            if (set_out_mode == -1) {
                fprintf(stderr, "%s: bad --out-mode optarg\n", prog);
                exit(1);
            }
            todo++;
            break;
        case 'g': /* --get-out-ampl */
            get_out_ampl = 1;
            todo++;
            break;
        case 'G': /* --set-out-ampl */
            set_out_ampl = strtod(optarg, NULL);
            todo++;
            break;
        case 'f' : /* --get-out-offset */
            get_out_offset = 1;
            todo++;
            break;
        case 'F': /* --set-out-offset */
            set_out_offset = strtod(optarg, NULL);
            todo++;
            break;
        case 'p': /* --get-out-polarity */
            get_out_polarity = 1;
            todo++;
            break;
        case 'P': /* --set-out-polarity */
            if (!strcasecmp(optarg, "-"))
                set_out_polarity = 0;
            else if (!strcasecmp(optarg, "+"))
                set_out_polarity = 1;
            else
                usage();
            todo++;
            break;
        case 'y': /* --get-out-z */
            get_out_z = 1;
            todo++;
            break;
        case 'Y': /* --set-out-z */
            if (!strcasecmp(optarg, "lo"))
                set_out_z = 0;
            else if (!strcasecmp(optarg, "hi"))
                set_out_z = 1;
            else
                usage();
            todo++;
            break;
        case 'm' : /* --get-trig-mode */
            get_trig_mode = 1;
            todo++;
            break;
        case 'M' : /* --set-trig-mode */
            set_trig_mode = rfindstr(trig_modes, optarg);
            if (set_trig_mode == -1) {
                fprintf(stderr, "%s: bad --trig-mode optarg\n", prog);
                exit(1);
            }
            todo++;
            break;
        case 't': /* --get-trig-rate */
            get_trig_rate = 1;
            todo++;
            break;
        case 'T': /* --set-trig-rate */
            if (freqstr(optarg, &set_trig_rate) < 0) {
                fprintf(stderr, "%s: error converting trigger freq\n", prog);
                fprintf(stderr, "%s: use freq units: %s\n", prog, FREQ_UNITS);
                fprintf(stderr, "%s: or period units: %s\n", prog,PERIOD_UNITS);
                exit(1);
            }
            todo++;
            break;
        case 's' : /* --get-trig-slope */
            get_trig_slope = 1;
            todo++;
            break;
        case 'S' : /* --set-trig-slope */
            if (!strcasecmp(optarg, "rising"))
                set_trig_slope = 1;
            else if (!strcasecmp(optarg, "falling"))
                set_trig_slope = 0;
            else
                usage();
            todo++;
            break;
        case 'b' : /* --get-trig-count */
            get_burst_count = 1;
            todo++;
            break;
        case 'B' : /* --set-trig-count */
            set_burst_count = strtoul(optarg, NULL, 0);
            todo++;
            break;
        case 'z' : /* --get-trig-z */
            get_trig_z = 1;
            todo++;
            break;
        case 'Z' : /* --set-trig-z */
            if (!strcasecmp(optarg, "lo"))
                set_trig_z = 0;
            else if (!strcasecmp(optarg, "hi"))
                set_trig_z = 1;
            else
                usage();
            todo++;
            break;
        default:
            usage();
            break;
        }
    }
    if (optind < argc || !todo)
        usage();

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
    if (out_chan == -1 && (get_delay || set_delay_chan != -1)) {
        fprintf(stderr, "%s: --get/set-delay needs --out-chan\n", prog);
        exit(1);
    }

    if (!addr)
        addr = gpib_default_addr(INSTRUMENT);
    if (!addr) {
        fprintf(stderr, "%s: no default address for %s, use --address\n", 
                prog, INSTRUMENT);
        exit(1);
    }
    gd = gpib_init(addr, _interpret_status, 0);
    if (!gd) {
        fprintf(stderr, "%s: device initialization failed for address %s\n", 
                prog, addr);
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
        int i;

        gpib_wrtf(gd, "%s %d", DG535_OUT_MODE, out_chan);
        if (gpib_rdf(gd, "%d", &i) != 1) {
            fprintf(stderr, "%s: error reading output mode\n", prog);
            exit_val = 1;
            goto done;
        }
        fprintf(stderr, "%s: %s\n", prog, findstr(out_modes, i));
    }
    if (set_out_ampl != HUGE_VALF)
        gpib_wrtf(gd, "%s %d,%lf", DG535_OUT_AMPL, out_chan, set_out_ampl);
    if (get_out_ampl) {
        double d;

        gpib_wrtf(gd, "%s %d", DG535_OUT_AMPL, out_chan);
        if (gpib_rdf(gd, "%lf", &d) != 1) {
            fprintf(stderr, "%s: error reading output amplitude\n", prog);
            exit_val = 1;
            goto done;
        }
        fprintf(stderr, "%s: %lf\n", prog, d);
    }
    if (set_out_offset != HUGE_VALF)
        gpib_wrtf(gd, "%s %d,%lf", DG535_OUT_OFFSET, out_chan, set_out_offset);
    if (get_out_offset) {
        double d;

        gpib_wrtf(gd, "%s %d", DG535_OUT_OFFSET, out_chan);
        if (gpib_rdf(gd, "%lf", &d) != 1) {
            fprintf(stderr, "%s: error reading output offset\n", prog);
            exit_val = 1;
            goto done;
        }
        fprintf(stderr, "%s: %lf\n", prog, d);
    }
    if (set_out_polarity != -1)
        gpib_wrtf(gd, "%s %d,%d", DG535_OUT_POLARITY, out_chan, set_out_polarity);
    if (get_out_polarity) {
        int i;

        gpib_wrtf(gd, "%s %d", DG535_OUT_POLARITY, out_chan);
        if (gpib_rdf(gd, "%d", &i) != 1) {
            fprintf(stderr, "%s: error reading output polarity\n", prog);
            exit_val = 1;
            goto done;
        }
        fprintf(stderr, "%s: %s\n", prog, i == 0 ? "-" : "+");
    }
    if (set_out_z != -1)
        gpib_wrtf(gd, "%s %d,%d", DG535_TERM_Z, out_chan, set_out_z);
    if (get_out_z) {
        int i;

        gpib_wrtf(gd, "%s %d", DG535_TERM_Z, out_chan);
        if (gpib_rdf(gd, "%d", &i) != 1) {
            fprintf(stderr, "%s: error reading output impedance\n", prog);
            exit_val = 1;
            goto done;
        }
        fprintf(stderr, "%s: %s\n", prog, i == 0 ? "lo" : "hi");
    }

    /* delay */
    if (set_delay_chan != -1) {
        gpib_wrtf(gd, "%s %d,%d,%lf", DG535_DELAY_TIME, out_chan,
                set_delay_chan, set_delay_time);
    }
    if (get_delay) {
        int i;
        double d;

        gpib_wrtf(gd, "%s %d", DG535_DELAY_TIME, out_chan);
        if (gpib_rdf(gd, "%d,%lf", &i, &d) != 2) {
            fprintf(stderr, "%s: error reading delay time\n", prog);
            exit_val = 1;
            goto done;
        }
        fprintf(stderr, "%s: %s,%lfs\n", prog, findstr(out_names, i), d);
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
