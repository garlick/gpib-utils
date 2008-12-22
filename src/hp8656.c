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
#include <math.h>

#include "hp8656.h"
#include "util.h"
#include "gpib.h"

#define INSTRUMENT "hp8656"

char *prog = "";
static int verbose = 0;

#define OPTIONS "a:f:a:vclF:i:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"incrfreq",        required_argument, 0, 'F'},
    {"frequency",       required_argument, 0, 'f'},
    {"incrampl",        required_argument, 0, 'i'},
    {"amplitude",       required_argument, 0, 'A'},
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
"  -a,--address                  instrument address [%s]\n"
"  -c,--clear                    initialize instrument to default values\n"
"  -l,--local                    return instrument to local operation on exit\n"
"  -v,--verbose                  show protocol on stderr\n"
"  -f,--frequency [value|up|dn]  set carrier freq [100Mhz]\n"
"  -A,--amplitude [value|up|dn]  set carrier amplitude [-127dBm]\n"
"  -F,--incrfreq value           set carrier freq increment [10.0MHz]\n"
"  -i,--incrampl value           set carrier amplitude increment [10.0dB]\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

int
main(int argc, char *argv[])
{
    char *addr = NULL;
    gd_t gd;
    char cmdstr[1024] = "";
    double f, a;
    int c;
    int clear = 0;
    int local = 0;

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
            break;
        case 'l': /* --local */
            local = 1;
            break;
        case 'F': /* --incrfreq */
            if (freqstr(optarg, &f) < 0) {
                fprintf(stderr, "%s: error parsing incrfreq arg\n", prog);
                fprintf(stderr, "%s: use freq units: %s\n", prog, FREQ_UNITS);
                fprintf(stderr, "%s: or period units: %s\n", prog,PERIOD_UNITS);
                exit(1);
            }
            if (f > 989.99E6 || f < 0.1E3) {
                fprintf(stderr, 
                        "%s: incrfreq out of range (0.1kHz-989.99MHz)\n", prog);
                exit(1);
            }
            if (fmod(f, 100) != 0 && fmod(f, 250) != 0) {
                fprintf(stderr, 
                        "%s: incrfreq must be a multiple of 100Hz or 250Hz\n",
                        prog);
                exit(1);
            }
            sprintf(cmdstr + strlen(cmdstr), "%s%s%.0lf%s", 
                HP8656_FREQ, HP8656_INCR_SET, f, HP8656_UNIT_HZ);
            break;
        case 'f': /* --frequency */
            if (!strcasecmp(optarg, "up")) {
                sprintf(cmdstr + strlen(cmdstr), "%s%s", 
                        HP8656_FREQ, HP8656_STEP_UP);
            } else if (!strcasecmp(optarg, "dn") ||!strcasecmp(optarg, "down")){
                sprintf(cmdstr + strlen(cmdstr), "%s%s", 
                        HP8656_FREQ, HP8656_STEP_DN); 
            } else {
                if (freqstr(optarg, &f) < 0) {
                    fprintf(stderr, "%s: error parsing frequency arg\n", prog);
                    fprintf(stderr, "%s: use freq units: %s\n", prog, 
                            FREQ_UNITS);
                    fprintf(stderr, "%s: or period units: %s\n", prog,
                            PERIOD_UNITS);
                    exit(1);
                }
                if (f < 100E3 || f > 990E6) {
                    fprintf(stderr, "%s: freq out of range (100kHz-990MHz)\n",prog);
                    exit(1);
                }
                sprintf(cmdstr + strlen(cmdstr), "%s%.0lf%s", 
                        HP8656_FREQ, f, HP8656_UNIT_HZ);
            }
            break;
        case 'i': /* --incrampl */
            /* XXX instrument accepts '%' units here too */
            if (amplstr(optarg, &a) < 0)  {
                char *end;
                double a = strtod(optarg, &end);

                if (strcasecmp(end, "db") != 0) {
                    fprintf(stderr, "%s: error parsing incrampl arg\n", prog);
                    fprintf(stderr, "%s: use log ampl units: db, %s\n", prog, 
                            AMPL_LOG_UNITS);
                    fprintf(stderr, "%s: or linear ampl units: %s\n", prog, 
                            AMPL_LIN_UNITS);
                    exit(1);
                }
                if (a < 0.1 || a > 144.0) {
                    fprintf(stderr, 
                            "%s: incrampl out of range (0.1dB-144.0db)\n", 
                            prog);
                    exit(1);
                }
                sprintf(cmdstr + strlen(cmdstr), "%s%s%.2lf%s", 
                    HP8656_AMPL, HP8656_INCR_SET, a, HP8656_UNIT_DB);
            } else {
                /* XXX instrument ignored minus sign when dbm units used here */
                a = dbmtov(a);  /* convert dbm to volts */
                if (a < 0.01E-6 || a > 1.57) {
                    fprintf(stderr, 
                            "%s: incrampl out of range (0.01uV-1.57V)\n", prog);
                    exit(1);
                }
                sprintf(cmdstr + strlen(cmdstr), "%s%s%.2lf%s", 
                    HP8656_AMPL, HP8656_INCR_SET, a*1E6, HP8656_UNIT_UV);
            }
            break;
        case 'A': /* --amplitude */
            if (!strcasecmp(optarg, "up")) {
                sprintf(cmdstr + strlen(cmdstr), "%s%s", 
                        HP8656_AMPL, HP8656_STEP_UP);
            } else if (!strcasecmp(optarg, "dn") ||!strcasecmp(optarg, "down")){
                sprintf(cmdstr + strlen(cmdstr), "%s%s", 
                        HP8656_AMPL, HP8656_STEP_DN); 
            } else {
                if (amplstr(optarg, &a) < 0) {
                    fprintf(stderr, "%s: error parsing amplitude arg\n", prog);
                    fprintf(stderr, "%s: use log ampl units: %s\n", prog, 
                            AMPL_LOG_UNITS);
                    fprintf(stderr, "%s: or linear ampl units: %s\n", prog, 
                            AMPL_LIN_UNITS);
                    exit(1);
                }
                if (a > 17.0) {
                    fprintf(stderr, "%s: amplitude exceeds range\n", prog);
                    exit(1);
                }
                if (a < -127.0 || a > 13.0)
                    fprintf(stderr, "%s: warning: amplitude beyond cal\n", 
                            prog);
                sprintf(cmdstr + strlen(cmdstr), "%s%.2lf%s", 
                        HP8656_AMPL, a, HP8656_UNIT_DBM);
            }
            break;
        case 'v': /* --verbose */
            verbose = 1;
            break;
        default:
            usage();
            break;
        }
    }

    if (!clear && !*cmdstr && !local)
        usage();

    if (!addr)
        addr = gpib_default_addr(INSTRUMENT);
    if (!addr) {
        fprintf(stderr, "%s: no default address for %s, use --address\n",
                prog, INSTRUMENT);
        exit(1);
    }
    gd = gpib_init(addr, NULL, 0);
    if (!gd) {
        fprintf(stderr, "%s: device initialization failed for address %s\n", 
                prog, addr);
        exit(1);
    }
    gpib_set_verbose(gd, verbose);

    /* clear nstrument to default settings */
    if (clear) {
        gpib_clr(gd, 1000000);
    }

    /* write cmd if specified */
    if (strlen(cmdstr) > 0)
        gpib_wrtf(gd, "%s", cmdstr);

    /* Sleep 2s to accomodate worst case settling time.
     * FIXME: The actual settling time should be calculated based 
     * on info in the manual.
     */
    if (cmdstr || clear)
        sleep(2);

    /* return front panel if requested */
    if (local)
        gpib_loc(gd); 

    gpib_fini(gd);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
