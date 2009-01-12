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

/* Stanford Research Systems DG535
 * Digital Delay / Pulse Generator.
 */

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
#if HAVE_GETOPT_LONG
#include <getopt.h>
#endif
#include <assert.h>
#include <sys/time.h>
#include <math.h>
#include <stdint.h>

#include "gpib.h"
#include "util.h"

#define INSTRUMENT "dg535"

#define DG535_DO_TRIG                   0       /* trigger input */
#define DG535_DO_T0                     1       /* T0 output */
#define DG535_DO_A                      2       /* A output */
#define DG535_DO_B                      3       /* B output */
#define DG535_DO_AB                     4       /* AB and -AB outputs */
#define DG535_DO_C                      5       /* C output */
#define DG535_DO_D                      6       /* D output */
#define DG535_DO_CD                     7       /* CD and -CD output */

#define DG535_OUT_TTL                   0
#define DG535_OUT_NIM                   1
#define DG535_OUT_ECL                   2
#define DG535_OUT_VAR                   3

#define DG535_TRIG_INT                  0
#define DG535_TRIG_EXT                  1
#define DG535_TRIG_SS                   2
#define DG535_TRIG_BUR                  3

/* Error status byte values.
 */
#define DG535_ERR_RECALL                0x40    /* recalled data was corrupt */
#define DG535_ERR_DELAY_RANGE           0x20    /* delay range error */
#define DG535_ERR_DELAY_LINKAGE         0x10    /* delay linkage error */
#define DG535_ERR_CMDMODE               0x08    /* wrong mode for command */
#define DG535_ERR_ERANGE                0x04    /* value out of range */
#define DG535_ERR_NUMPARAM              0x02    /* wrong number of paramters */
#define DG535_ERR_BADCMD                0x01    /* unrecognized command */

/* Instrument status byte values.
 * These bits can generate SRQ if present in the status mask.
 */
#define DG535_STAT_BADMEM               0x40    /* memory contents corrupted */
#define DG535_STAT_SRQ                  0x20    /* service request */
#define DG535_STAT_TOOFAST              0x10    /* trigger rate too high */
#define DG535_STAT_PLL                  0x08    /* 80MHz PLL is unlocked */
#define DG535_STAT_TRIG                 0x04    /* trigger has occurred */
#define DG535_STAT_BUSY                 0x02    /* busy with timing cycle */
#define DG535_STAT_CMDERR               0x01    /* command error detected */

static void _usage(void);
static int _interpret_status(gd_t gd, unsigned char status, char *msg);
static int _set_delay(gd_t gd, int out_chan, char *str);

char *prog = "";

static strtab_t out_names[] = {
    { DG535_DO_T0, "t0" },
    { DG535_DO_A,  "a" },
    { DG535_DO_B,  "b" },
    { DG535_DO_AB, "ab" },
    { DG535_DO_C,  "c" },
    { DG535_DO_D,  "d" },
    { DG535_DO_CD, "cd" },
    { 0, NULL },
};
static strtab_t out_modes[] = {
    { DG535_OUT_TTL, "ttl" },
    { DG535_OUT_NIM, "nim" },
    { DG535_OUT_ECL, "ecl" },
    { DG535_OUT_VAR, "var" },
    { 0, NULL },
};
static strtab_t trig_modes[] = {
    { DG535_TRIG_INT, "int" },
    { DG535_TRIG_EXT, "ext" },
    { DG535_TRIG_SS,  "ss" },
    { DG535_TRIG_BUR, "bur" },
    { 0, NULL },
};
static strtab_t impedance[] = {
    { 0, "lo" },
    { 1, "hi" },
    { 0, NULL },
};
static strtab_t slope[] = {
    { 0, "falling" },
    { 1, "rising" },
    { 0, NULL },
};
static strtab_t polarity[] = {
    { 0, "-" },
    { 1, "+" },
    { 0, NULL },
};

#define OPTIONS "a:clve:o:dD:qQ:gG:fF:pP:yY:tT:mM:sS:bB:zZ:D:x"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
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
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int
main(int argc, char *argv[])
{
    int c, out_chan = -1;
    int tmpi, exit_val = 0;
    int print_usage = 0, out_chan_needed = 0;
    double tmpd;
    char tmpstr[1024];
    gd_t gd;

    gd = gpib_init_args(argc, argv, OPTIONS, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage)
        _usage();
    if (!gd)
        exit(1);

    /* preparse args to get output channel */
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'o': /* --out-chan */
                out_chan = rfindstr(out_names, optarg);
                if (out_chan == -1) {
                    fprintf(stderr, "%s: bad --out-chan argument\n", prog);
                    exit_val = 1;
                    goto done;
                }
            case 'q':
            case 'Q':
            case 'g':
            case 'G':
            case 'f':
            case 'F':
            case 'p':
            case 'P':
            case 'y':
            case 'Y':
            case 'd':
            case 'D':
                out_chan_needed++;
                break;
        }
    }
    if (out_chan == -1 && out_chan_needed) {
        fprintf(stderr, "%s: one or more arguments require --out-chan\n", prog);
        exit_val = 1;
        goto done;
    }

    optind = 0;
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            case 'a': /* -a, -v, -o already parsed */
            case 'v':
            case 'o':
                break;
            case 'c': /* --clear */
                gpib_clr(gd, 0);
                gpib_wrtf(gd, "CL");
                break;
            case 'l': /* --local */
                gpib_loc(gd); 
                break;
            case 'e': /* --display-string */
                gpib_wrtf(gd, "DS %s", optarg);
                break;
            case 'x': /* --single-shot */
                gpib_wrtf(gd, "SS");
                break;
            case 'd': /* --get-delay */
                gpib_wrtf(gd, "DT %d", out_chan);
                if (gpib_rdf(gd, "%d,%lf", &tmpi, &tmpd) != 2) {
                    fprintf(stderr, "%s: error parsing DT response\n", prog);
                    exit_val = 1;
                    break;
                }
                printf("delay: %s + %lfs\n", findstr(out_names, tmpi), tmpd);
                break;
            case 'D': /* --set-delay */
                if (!_set_delay(gd, out_chan, optarg))
                    exit_val = 1;
                break;
            case 'q' : /* --get-out-mode */
                gpib_wrtf(gd, "OM %d", out_chan);
                if (gpib_rdf(gd, "%d", &tmpi) != 1) {
                    fprintf(stderr, "%s: error parsing OM response\n", prog);
                    exit_val = 1;
                    break;
                }
                printf("output mode: %s\n", findstr(out_modes, tmpi));
                break;
            case 'Q': /* --set-out-mode */
                gpib_wrtf(gd, "OM %d,%s", out_chan, rfindstr(out_modes, optarg));
                break;
            case 'g': /* --get-out-ampl */
                gpib_wrtf(gd, "OA %d", out_chan);
                gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
                printf("output amplitude: %s\n", tmpstr);
                break;
            case 'G': /* --set-out-ampl */
                gpib_wrtf(gd, "OA %d,%s", out_chan, optarg);
                break;
            case 'f' : /* --get-out-offset */
                gpib_wrtf(gd, "OO %d", out_chan);
                gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
                printf("output offset: %s\n", tmpstr);
                break;
            case 'F': /* --set-out-offset */
                gpib_wrtf(gd, "OO %d,%s", out_chan, optarg);
                break;
            case 'p': /* --get-out-polarity */
                gpib_wrtf(gd, "OP %d", out_chan);
                if (gpib_rdf(gd, "%d", &tmpi) != 1) {
                    fprintf(stderr, "%s: error parsing OP response\n", prog);
                    exit_val = 1;
                    break;
                }
                printf("output polarity: %s\n", findstr(polarity, tmpi));
                break;
            case 'P': /* --set-out-polarity */
                gpib_wrtf(gd, "OP %d,%d", out_chan, rfindstr(polarity, optarg));
                break;
            case 'y': /* --get-out-z */
                gpib_wrtf(gd, "TZ %d", out_chan);
                if (gpib_rdf(gd, "%d", &tmpi) != 1) {
                    fprintf(stderr, "%s: error parsing TZ response\n", prog);
                    exit_val = 1;
                    break;
                }
                printf("output impedance: %s\n", findstr(impedance, tmpi));
                break;
            case 'Y': /* --set-out-z */
                gpib_wrtf(gd, "TZ %d,%d", rfindstr(impedance, optarg));
                break;
            case 'm' : /* --get-trig-mode */
                gpib_wrtf(gd, "TM");
                if (gpib_rdf(gd, "%d", &tmpi) != 1) {
                    fprintf(stderr, "%s: error parsing TM response\n", prog);
                    exit_val = 1;
                    break;
                }
                printf("trigger mode: %s\n", findstr(trig_modes, tmpi));
                break;
            case 'M' : /* --set-trig-mode */
                gpib_wrtf(gd, "TM %d", rfindstr(trig_modes, optarg));
                break;
            case 't': /* --get-trig-rate */
                gpib_wrtf(gd, "TM");
                if (gpib_rdf(gd, "%d", &tmpi) != 1) {
                    fprintf(stderr, "%s: error parsing TM response\n", prog);
                    exit_val = 1;
                    break;
                }
                gpib_wrtf(gd, "TR %d", tmpi == DG535_TRIG_INT ? 0 : 1);
                if (gpib_rdf(gd, "%lf", &tmpd) != 1) {
                    fprintf(stderr, "%s: error parsing TR response\n", prog);
                    exit_val = 1;
                    break;
                }
                printf("trigger rate: %lfhz\n", tmpd);
                break;
            case 'T': /* --set-trig-rate */
                if (freqstr(optarg, &tmpd) < 0) {
                    fprintf(stderr, "%s: error parsing trigger rate\n", prog);
                    fprintf(stderr, "%s: use units of: %s\n", prog, FREQ_UNITS);
                    exit_val = 1;
                    break;
                }
                gpib_wrtf(gd, "TM");
                if (gpib_rdf(gd, "%d", &tmpi) != 1) {
                    fprintf(stderr, "%s: error parsing TM response\n", prog);
                    exit_val = 1;
                    break;
                }
                gpib_wrtf(gd, "TR %d,%lf", tmpi==DG535_TRIG_INT ? 0 : 1, tmpd);
                break;
            case 's' : /* --get-trig-slope */
                gpib_wrtf(gd, "TS", out_chan);
                if (gpib_rdf(gd, "%d", &tmpi) != 1) {
                    fprintf(stderr, "%s: error parsing TZ response\n", prog);
                    exit_val = 1;
                } else
                    printf("trigger slope: %s\n", findstr(slope, tmpi));
                break;
            case 'S' : /* --set-trig-slope */
                gpib_wrtf(gd, "TS %d", rfindstr(slope, optarg));
                break;
            case 'b' : /* --get-trig-count */
                gpib_wrtf(gd, "BC");
                gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
                printf("burst count: %s\n", tmpstr);
                break;
            case 'B' : /* --set-trig-count */
                gpib_wrtf(gd, "BC %s", optarg);
                break;
            case 'z' : /* --get-trig-z */
                gpib_wrtf(gd, "TZ 0", out_chan);
                if (gpib_rdf(gd, "%d", &tmpi) != 1) {
                    fprintf(stderr, "%s: error parsing TZ response\n", prog);
                    exit_val = 1;
                    break;
                }
                printf("trigger impedance: %s\n", findstr(impedance, tmpi));
                break;
            case 'Z' : /* --set-trig-z */
                gpib_wrtf(gd, "TZ 0,%d", rfindstr(impedance, optarg));
                break;
        }
    }

done:
    gpib_fini(gd);
    exit(exit_val);
}

static void 
_usage(void)
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
"  -dD,--get/set-delay         output delay (another-chan,delay)\n"
"  -qQ,--get/set-out-mode      output mode (ttl|nim|ecl|var)\n"
"  -gG,--get/set-out-ampl      output amplitude (-4:-0.1, +0.1:+4) volts\n"
"  -fF,--get/set-out-offset    output offset (-4:+4) volts\n"
"  -pP,--get/set-out-polarity  output polarity (+|-)\n"
"  -yY,--get/set-out-z         output impedance (hi|lo)\n"
"  -mM,--get/set-trig-mode     trigger mode (int|ext|ss|bur)\n"
"  -tT,--get/set-trig-rate     trigger rate (0.001hz:1.000mhz)\n"
"  -sS,--get/set-trig-slope    trigger slope (rising|falling)\n"
"  -bB,--get/set-burst-count   trigger burst count (2:19)\n"
"  -zZ,--get/set-trig-z        trigger input impedance (hi|lo)\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

static void
_check_err(gd_t gd)
{
    char buf[16];
    int status;

    /* check error status */
    gpib_wrtf(gd, "ES");
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
_set_delay(gd_t gd, int out_chan, char *str)
{
    double f;
    char *cstr, *dstr;
    int follow_chan;
    double delay;

    if (!(cstr = strtok(str, ",")))
        return 0;
    if (!(dstr = strtok(NULL, ",")))
        return 0;
    if ((follow_chan = rfindstr(out_names, cstr)) == -1)
        return 0;
    if (freqstr(dstr, &f) == -1) {
        fprintf(stderr, "%s: specify time units in s, ms, us, ns, ps\n", prog);
        return 0;
    }
    delay = 1.0/f;
    gpib_wrtf(gd, "DT %d,%d,%lf", out_chan, follow_chan, delay);
    return 1;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
