/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2008-2009 Jim Garlick <garlick.jim@gmail.com>

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

/* HP 6632A (25V, 4A); 6633A (50V, 2A); 6634A (100V, 1A) power supplies */

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
#include <math.h>
#include <stdint.h>

#include "util.h"
#include "gpib.h"

#define INSTRUMENT "hp6632"

/* serial poll reg */
#define HP6032_SPOLL_FAU    1
#define HP6032_SPOLL_PON    2
#define HP6032_SPOLL_RDY    16
#define HP6032_SPOLL_ERR    32
#define HP6032_SPOLL_RQS    64

/* status reg. bits (STS? - also ASTS? UNMASK? FAULT?) */
#define HP6032_STAT_CV      1       /* constant voltage mode */
#define HP6032_STAT_CC      2       /* positive constant current mode */
#define HP6032_STAT_UNR     4       /* output unregulated */
#define HP6032_STAT_OV      8       /* overvoltage protection tripped */
#define HP6032_STAT_OT      16      /* over-temperature protection tripped */
#define HP6032_STAT_OC      64      /* overcurrent protection tripped */
#define HP6032_STAT_ERR     128     /* programming error */
#define HP6032_STAT_INH     256     /* remote inhibit */
#define HP6032_STAT_NCC     512     /* negative constant current mode */
#define HP6032_STAT_FAST    1024    /* output in fast operating mode */
#define HP6032_STAT_NORM    2048    /* output in normal operating mode */

/* programming errors */
static strtab_t petab[] = {
    { 0, "Success" },
    { 1, "EEPROM save failed" },
    { 2, "Second PON after power-on" },
    { 4, "Second DC PON after power-on" },
    { 5, "No relay option present" },
    { 8, "Addressed to talk and nothing to say" },
    { 10, "Header expected" },
    { 11, "Unrecognized header" },
    { 20, "Number expected" },
    { 21, "Number syntax" },
    { 22, "Number out of internal range" },
    { 30, "Comma expected" },
    { 31, "Terminator expected" },
    { 41, "Parameter out of range" },
    { 42, "Voltage programming error" },
    { 43, "Current programming error" },
    { 44, "Overvoltage programming error" },
    { 45, "Delay programming error" },
    { 46, "Mask programming error" },
    { 50, "Multiple CSAVE commands" },
    { 51, "EEPROM checksum failure" },
    { 52, "Calibration mode disabled" },
    { 53, "CAL channel out of range" },
    { 54, "CAL FS out of range" },
    { 54, "CAL offset out of range" },
    { 54, "CAL disable jumper in" },
    { 0, NULL }
};

/* selftest failures */
static strtab_t sttab[] = {
    { 0, "Passed" },
    { 1, "ROM checksum failure" },
    { 2, "RAM test failure" },
    { 3, "HP-IB chip failure" },
    { 4, "HP-IB microprocessor timer slow" },
    { 5, "HP-IB microprocessor timer fast" },
    { 11, "PSI ROM checksum failure" },
    { 12, "PSI RAM test failure" },
    { 14, "PSI microprocessor timer slow" },
    { 15, "PSI microprocessor timer fast" },
    { 16, "A/D test reads high" },
    { 17, "A/D test reads low" },
    { 18, "CV/CC zero too high" },
    { 19, "CV/CC zero too low" },
    { 20, "CV ref FS too high" },
    { 21, "CV ref FS too low" },
    { 22, "CC ref FS too high" },
    { 23, "CC ref FS too low" },
    { 24, "DAC test failed" },
    { 51, "EEPROM checksum failed" },
    { 0, NULL }
};

static void _usage(void);
static int _interpret_status(gd_t gd, unsigned char status, char *msg);
static int _readiv(gd_t gd, double *ip, double *vp);

char *prog = "";

#define OPTIONS "a:clviSI:V:o:O:C:Rs:p:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"verbose",         no_argument,       0, 'v'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"get-idn",         no_argument,       0, 'i'},
    {"get-rom",         no_argument,       0, 'R'},
    {"selftest",        no_argument,       0, 'S'},
    {"iset",            required_argument, 0, 'I'},
    {"vset",            required_argument, 0, 'V'},
    {"out",             required_argument, 0, 'o'},
    {"ovset",           required_argument, 0, 'O'},
    {"ocp",             required_argument, 0, 'C'},
    {"samples",         required_argument, 0, 's'},
    {"period",          required_argument, 0, 'p'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

int
main(int argc, char *argv[])
{
    gd_t gd;
    int rc, c, print_usage = 0;
    int exit_val = 0;
    char tmpstr[64];
    int samples = 0;
    double period = 0.0;
    int showtime = 0;

    gd = gpib_init_args(argc, argv, OPTIONS, longopts, INSTRUMENT,
                        _interpret_status, 0, &print_usage);
    if (print_usage)
        _usage();
    if (!gd)
        exit(1);

    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
        case 'a': /* -a and -v handled in gpib_init_args */
        case 'v':
            break;
        case 'c': /* --clear */
            gpib_clr(gd, 1000000); /* same as 'CLR' command */
            gpib_wrtf(gd, "OUT 0;RST\n");
            sleep(1);
            break;
        case 'l': /* --local */
            gpib_loc(gd); 
            break;
        case 'i': /* --get-idn */
            gpib_wrtstr(gd, "ID?\n");
            gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
            printf("%s\n", tmpstr);
            break;
        case 'R': /* --get-rom */
            gpib_wrtstr(gd, "ROM?\n");
            gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
            printf("rom version: %s\n", tmpstr);
            break;
        case 'S': /* --selftest */
            gpib_wrtstr(gd, "TEST?\n");
            gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
            if (sscanf(tmpstr, "%d", &rc) == 1)
                printf("self-test: %s\n", findstr(sttab ,rc));
            else
                printf("self-test: %s\n", tmpstr);
            break;
        case 'I': /* --iset */
            gpib_wrtf(gd, "ISET %s\n", optarg);
            break;
        case 'V': /* --vset */
            gpib_wrtf(gd, "VSET %s\n", optarg);
            break;
        case 'O': /* --ovset */
            gpib_wrtf(gd, "OVSET %s\n", optarg);
            break;
        case 'o': /* --out */
            gpib_wrtf(gd, "OUT %s\n", optarg);
            break;
        case 'C': /* --ocp */
            gpib_wrtf(gd, "OCP %s\n", optarg);
            break;
        case 's': /* --samples */
            samples = strtoul(optarg, NULL, 10);
            if (samples > 1)
                showtime = 1;
            break;
        case 'p': /* --period */
            if (freqstr(optarg, &period) < 0) {
                fprintf(stderr, "%s: error parsing period argument\n", prog);
                fprintf(stderr, "%s: use units of: %s\n", prog, PERIOD_UNITS);
                exit_val = 1;
                break;
            }
            period = 1.0/period;
            break;
        }
    }

    if (samples > 0) {
        double t0, t1, t2;
        double i, v;

        t0 = 0;
        while (samples-- > 0) {
            t1 = gettime();
            if (t0 == 0)
                t0 = t1;
            if (_readiv(gd, &i, &v) == -1) {
                exit_val = 1;
                goto done;  
            }
            t2 = gettime();

            if (showtime)
                printf("%-3.3lf\t%lf\t%lf\n", (t1-t0), i, v);
            else
                printf("%lf\t%lf\n", i, v);
            if (samples > 0 && period > 0)
                sleep_sec(period - (t2-t1));
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
"  -a,--address            set instrument address [%s]\n"
"  -c,--clear              initialize instrument to default values\n"
"  -l,--local              return instrument to local operation on exit\n"
"  -v,--verbose            show protocol on stderr\n"
"  -i,--get-idn            print instrument model\n"
"  -R,--get-rom            print instrument ROM version\n"
"  -S,--selftest           run selftest\n"
"  -I,--iset               set current\n"
"  -V,--vset               set voltage\n"
"  -o,--out                enable/disable output (0|1)\n"
"  -O,--ovset              set overvoltage threshold\n"
"  -C,--ocp                enable/disable overcurrent protection (0|1)\n"
"  -s,--samples            number of samples [0]\n"
"  -p,--period             sample period\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    char tmpstr[64];    
    int e, err = 0;

    if (status & HP6032_SPOLL_FAU) {
        fprintf(stderr, "%s: device fault\n", prog);
        /* FIXME: check FAULT? and/or ASTS? but only if unmasked */
        err = 1;
    }
    if (status & HP6032_SPOLL_PON) {
        /*fprintf(stderr, "%s: power-on detected\n", prog);*/
    }
    if (status & HP6032_SPOLL_RDY) {
    }
    if (status & HP6032_SPOLL_ERR) {
        gpib_wrtstr(gd, "ERR?\n");
        gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
        if (sscanf(tmpstr, "%d", &e) == 1)
            fprintf(stderr, "%s: prog error: %s\n", prog, findstr(petab, e));
        else
            fprintf(stderr, "%s: prog error: %s\n", prog, tmpstr);
        err = 1;
    }
    if (status & HP6032_SPOLL_RQS) {
        fprintf(stderr, "%s: device is requesting service\n", prog);
        err = 1; /* it shouldn't be unless we tell it to */
    }
    return err;
}

static int 
_readiv(gd_t gd, double *ip, double *vp)
{
    gpib_wrtstr(gd, "VOUT?\n");
    if (gpib_rdf(gd, "%lf", vp) != 1) {
        fprintf(stderr, "%s: error reading VOUT? result\n", prog);
        return -1;
    }
    gpib_wrtstr(gd, "IOUT?\n");
    if (gpib_rdf(gd, "%lf", ip) != 1) {
        fprintf(stderr, "%s: error reading IOUT? result\n", prog);
        return -1;
    }
    return 0;
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
