/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2007-2009 Jim Garlick <garlick.jim@gmail.com>

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

/* Agilent 34401A DMM */
/* Agilent 34410A and 34411A DMM  */

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

#include "gpib.h"
#include "util.h"

#define INSTRUMENT "a34401"

/* Status byte values
 */
#define A34401_STAT_RQS  64      /* device is requesting service */
#define A34401_STAT_SER  32      /* bits are set in std. event register */
#define A34401_STAT_MAV  16      /* message(s) available in the output buffer */
#define A34401_STAT_QDR  8       /* bits are set in quest. data register */

/* Standard event register values.
 */
#define A34401_SER_PON   128     /* power on */
#define A34401_SER_CME   32      /* command error */
#define A34401_SER_EXE   16      /* execution error */
#define A34401_SER_DDE   8       /* device dependent error */
#define A34401_SER_QYE   4       /* query error */
#define A34401_SER_OPC   1       /* operation complete */

/* Questionable data register.
 */
#define A34401_QDR_LFH   4096    /* limit fail HI */
#define A34401_QDR_LFL   2048    /* limit fail LO */
#define A34401_QDR_ROL   512     /* ohms overload */
#define A34401_QDR_AOL   2       /* current overload */
#define A34401_QDR_VOL   1       /* voltage overload */

#define SETUP_STR_SIZE  3000     /* 2617 actually */

char *prog = "";
static int verbose = 0;

#define OPTIONS "a:clviSr:R:f:t:s:p:zZ"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"get-idn",         no_argument,       0, 'i'},
    {"selftest",        no_argument,       0, 'S'},
    {"function",        required_argument, 0, 'f'},
    {"range",           required_argument, 0, 'r'},
    {"resolution",      required_argument, 0, 'R'},
    {"trigger",         required_argument, 0, 't'},
    {"samples",         required_argument, 0, 's'},
    {"period",          required_argument, 0, 'p'},
    {"save-setup",      no_argument,       0, 'z'},
    {"restore-setup",   no_argument,       0, 'Z'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static void 
usage(void)
{
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -a,--address               set instrument address [%s]\n"
"  -c,--clear                 initialize instrument to default values\n"
"  -l,--local                 return instrument to local op on exit\n"
"  -v,--verbose               show protocol on stderr\n"
"  -i,--get-idn               return instrument idn string\n"
"  -S,--selftest              perform instrument self-test\n"
"  -f,--function dcv|acv|dci|aci|ohm2|ohm4|freq|period\n"
"                             select function [dcv]\n"
"  -r,--range                 set range: range|MIN|MAX|DEF\n"
"  -R,--resolution            set resolution: resolution|MIN|MAX|DEF\n"
"  -t,--trigger imm|ext|bus   select trigger mode\n"
"  -s,--samples               number of samples [0]\n"
"  -p,--period                sample period\n"
"  -z,--save-setup            save setup to stdout\n"
"  -Z,--restore-setup         restore setup from stdin\n"
           , prog, addr ? addr : "no default");
    exit(1);
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


static void
_dump_error_queue(gd_t gd)
{
    char tmpstr[128];
    char *endptr;
    int err;

    do {
        gpib_wrtf(gd, ":SYSTEM:ERROR?");
        gpib_rdstr(gd, tmpstr, sizeof(tmpstr));
        err = strtoul(tmpstr, &endptr, 10);
        if (err)
            fprintf(stderr, "%s: system error: %s\n", prog, endptr + 1);
    } while (err);
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

    if (status & A34401_STAT_RQS) {
    }
    if (status & A34401_STAT_SER) {
    }
    if (status & A34401_STAT_MAV) {
    }
    if (status & A34401_STAT_QDR) {
    }

    return err;
}

/* Save setup to stdout.
 */
static int
_save_setup(gd_t gd)
{
    unsigned char *buf = xmalloc(SETUP_STR_SIZE);
    int len;

    len = gpib_qry(gd, "*LRN?", buf, SETUP_STR_SIZE);
    if (write_all(1, buf, len) < 0) {
        perror("write");
        return -1;
    }
    fprintf(stderr, "%s: save setup: %d bytes\n", prog, len);
    free(buf);
    return 0;
}

/* Restore setup from stdin.
 */
static int
_restore_setup(gd_t gd)
{
    unsigned char buf[SETUP_STR_SIZE];
    int len;

    len = read_all(0, buf, sizeof(buf));
    if (len < 0) {
        perror("read");
        return -1;
    }
    gpib_wrt(gd, buf, len);
    fprintf(stderr, "%s: restore setup: %d bytes\n", prog, len);
    return 0;
}

/* Print instrument idn string.
 */
static int
_get_idn(gd_t gd)
{
    char tmpstr[64];

    gpib_qry(gd, "*IDN?", tmpstr, sizeof(tmpstr) - 1);
    fprintf(stderr, "%s: %s", prog, tmpstr);
    return 0;
}

int
main(int argc, char *argv[])
{
    gd_t gd;
    char *addr = NULL;
    int c;
    int clear = 0;
    int local = 0;
    int todo = 0;
    int exit_val = 0;
    int get_idn = 0;
    int selftest = 0;
    int samples = 0;
    double period = 0.0;
    int showtime = 0;
    char *fun = NULL;
    char range[16] = "DEF";
    char resolution[16] = "DEF";
    int setrr = 0;
    char *trig = NULL;
    int save = 0;
    int restore = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
        case 'a': /* --address */
            addr = optarg;
            break;
        case 'c': /* --clear */
            clear = 1;
            todo++;
            break;
        case 'l': /* --local */
            local = 1;
            todo++;
            break;
        case 'v': /* --verbose */
            verbose = 1;
            break;
        case 'i': /* --get-idn */
            get_idn = 1;
            todo++;
            break;
        case 'S': /* --selftest */
            selftest = 1;
            todo++;
            break;
        case 'r': /* --range */
            snprintf(range, sizeof(range), "%s", optarg);
            todo++;
            setrr++;
            break;
        case 'R': /* --resolution */
            snprintf(resolution, sizeof(resolution), "%s", optarg);
            todo++;
            setrr++;
            break;
        case 'f': /* --function */
            if (!strcmp(optarg,      "dcv"))
                fun = "CONF:VOLT:DC";
            else if (!strcmp(optarg, "acv"))
                fun = "CONF:VOLT:AC";
            else if (!strcmp(optarg, "dci"))
                fun = "CONF:CURR:DC";
            else if (!strcmp(optarg, "aci"))
                fun = "CONF:CURR:AC";
            else if (!strcmp(optarg, "ohm2"))
                fun = "CONF:RES";
            else if (!strcmp(optarg, "ohm4"))
                fun = "CONF:FRES";
            else if (!strcmp(optarg, "freq"))
                fun = "CONF:FREQ";
            else if (!strcmp(optarg, "period"))
                fun = "CONF:PER";
            else
                usage();
            todo++;
            break;
        case 't': /* --trigger */
            if (!strcmp(optarg, "imm")) {
                trig = "TRIG:SOUR IMM";
            } else if (!strcmp(optarg, "ext")) {
                trig = "TRIG:SOUR EXT";
            } else if (!strcmp(optarg, "bus")) {
                trig = "TRIG:SOUR BUS";
            } else
                usage();
            todo++;
            break;
        case 's': /* --samples */
            samples = strtoul(optarg, NULL, 10);
            if (samples > 1)
                showtime = 1;
            todo++;
            break;
        case 'p': /* --period */
            if (freqstr(optarg, &period) < 0) {
                fprintf(stderr, "%s: error parsing period argument\n", prog);
                fprintf(stderr, "%s: use freq units: %s\n", prog, FREQ_UNITS);
                fprintf(stderr, "%s: or period units: %s\n", prog,PERIOD_UNITS);
                exit(1);
            }
            period = 1.0/period;
            break;
        case 'z': /* --save-setup */
            save = 1;
            todo++;
            break;
        case 'Z': /* --restore-setup */
            restore = 1;
            todo++;
            break;
        default:
            usage();
            break;
        }
    }
    if (optind < argc || !todo)
        usage();
    if (setrr && !fun) {
        fprintf(stderr, "%s: range/resolution require function\n", prog);
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

    if (clear) {
        gpib_clr(gd, 0);
        gpib_wrtf(gd, "*CLS");
        gpib_wrtf(gd, "*RST");
        sleep(1);
    }
    if (get_idn) {
        if (_get_idn(gd) == -1) {
            exit_val = 1;
            goto done;
        }
    }
    if (selftest) {
        char buf[64];

        gpib_qry(gd, "*TST?", buf, sizeof(buf));
        switch (strtoul(buf, NULL, 10)) {
            case 0:
                fprintf(stderr, "%s: self-test completed successfully\n", prog);
                break;
            case 1:
                fprintf(stderr, "%s: self-test failed\n", prog);
                break;
            default:
                fprintf(stderr, "%s: self-test returned unexpected response\n",
                        prog);
                break;
        }
    }

    if (save) {
        _save_setup(gd);
    }
    if (restore) {
        _restore_setup(gd);
    }

    if (fun) {
        gpib_wrtf(gd, "%s %s,%s", fun, range, resolution);
    }

    if (trig) {
        gpib_wrtf(gd, "%s", trig);
    }

    if (samples > 0) {
        char buf[64];
        double t0, t1, t2;

        t0 = 0;
        while (samples-- > 0) {
            t1 = _gettime();
            if (t0 == 0)
                t0 = t1;

            gpib_wrtf(gd, "%s", "READ?");
            gpib_rdstr(gd, buf, sizeof(buf));
            t2 = _gettime();

            if (showtime)
                printf("%-3.3lf\t%s\n", (t1-t0), buf);
            else
                printf("%s\n", buf);
            if (samples > 0 && period > 0)
                _sleep_sec(period - (t2-t1));
        }
    }

    if (local) {
        gpib_loc(gd); 
    }

done:
    gpib_fini(gd);
    exit(exit_val);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
