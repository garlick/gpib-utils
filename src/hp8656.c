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

/* This instrument is listen-only (no SRQ, no serial/parallel poll).
 */

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

#include "util.h"
#include "gpib.h"

#define INSTRUMENT "hp8656"

char *prog = "";

#define OPTIONS OPTIONS_COMMON "f:a:clF:i:"
#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    OPTIONS_COMMON_LONG,
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"incrfreq",        required_argument, 0, 'F'},
    {"frequency",       required_argument, 0, 'f'},
    {"incrampl",        required_argument, 0, 'i'},
    {"amplitude",       required_argument, 0, 'A'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static opt_desc_t optdesc[] = {
    OPTIONS_COMMON_DESC,
    {"c","clear",     "initialize instrument to default values"},
    {"l","local",     "return instrument to local operation on exit"},
    {"f","frequency", "set carrier freq [100Mhz]"},
    {"A","amplitude", "set carrier amplitude [-127dBm]"},
    {"F","incrfreq",  "set carrier freq increment [10.0MHz]"},
    {"i","incrampl"   "set carrier amplitude increment [10.0dB]"},
    {0,0}
};

int
main(int argc, char *argv[])
{
    gd_t gd;
    double f, a;
    int c;
    int print_usage = 0;

    gd = gpib_init_args(argc, argv, OPTIONS, longopts, INSTRUMENT,
                        NULL, 0, &print_usage);
    if (print_usage)
        usage(optdesc);
    if (!gd)
        exit(1);

    while ((c = GETOPT(argc, argv, OPTIONS, longopts)) != EOF) {
        switch (c) {
            OPTIONS_COMMON_SWITCH
                break;
            case 'c': /* --clear */
                gpib_clr(gd, 1000000);
                sleep(2); /* settling time (worst case) */
                break;
            case 'l': /* --local */
                gpib_loc(gd); 
                break;
            case 'F': /* --incrfreq */
                if (freqstr(optarg, &f) < 0) {
                    fprintf(stderr, "%s: error parsing incrfreq arg\n", prog);
                    fprintf(stderr, "%s: use freq units: %s\n", prog,
                            FREQ_UNITS);
                    fprintf(stderr, "%s: or period units: %s\n", prog,
                            PERIOD_UNITS);
                    exit(1);
                }
                if (f > 989.99E6 || f < 0.1E3) {
                    fprintf(stderr, 
                            "%s: incrfreq out of range (0.1kHz-989.99MHz)\n",
                            prog);
                    exit(1);
                }
                if (fmod(f, 100) != 0 && fmod(f, 250) != 0) {
                    fprintf(stderr, "%s: incrfreq must be a multiple of"
                            " 100Hz or 250Hz\n", prog);
                    exit(1);
                }
                gpib_wrtf(gd, "FRIS%.0lfHZ", f);
                sleep(2); /* settling time (worst case) */
                break;
            case 'f': /* --frequency */
                if (!strcasecmp(optarg, "up")) {
                    gpib_wrtf(gd, "FRUP");
                } else if (!strcasecmp(optarg, "dn") 
			|| !strcasecmp(optarg, "down")) {
                    gpib_wrtf(gd, "FRDN");
                } else {
                    if (freqstr(optarg, &f) < 0) {
                        fprintf(stderr, "%s: error parsing frequency arg\n",
                                prog);
                        fprintf(stderr, "%s: use freq units: %s\n", prog,
                                FREQ_UNITS);
                        fprintf(stderr, "%s: or period units: %s\n", prog,
                                PERIOD_UNITS);
                        exit(1);
                    }
                    if (f < 100E3 || f > 990E6) {
                        fprintf(stderr, "%s: freq out of range"
                                " (100kHz-990MHz)\n", prog);
                        exit(1);
                    }
                    gpib_wrtf(gd, "FR%.0lfHZ", f);
                }
                sleep(2); /* settling time (worst case) */
                break;
            case 'i': /* --incrampl */
                /* XXX instrument accepts '%' units here too */
                if (amplstr(optarg, &a) < 0)  {
                    char *end;
                    double a = strtod(optarg, &end);
    
                    if (strcasecmp(end, "db") != 0) {
                        fprintf(stderr, "%s: error parsing incrampl arg\n",
                                prog);
                        fprintf(stderr, "%s: use log ampl units: db, %s\n",
                                prog, AMPL_LOG_UNITS);
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
                    gpib_wrtf(gd, "APIS%.2lfDB", a);
                } else {
                    /* XXX instrument ignored (-) when dbm units used here */
                    a = dbmtov(a);  /* convert dbm to volts */
                    if (a < 0.01E-6 || a > 1.57) {
                        fprintf(stderr, 
                                "%s: incrampl out of range (0.01uV-1.57V)\n",
                                prog);
                        exit(1);
                    }
                    gpib_wrtf(gd, "APIS%.2lfUV", a*1E6);
                }
                sleep(2); /* settling time (worst case) */
                break;
            case 'A': /* --amplitude */
                if (!strcasecmp(optarg, "up")) {
                    gpib_wrtf(gd, "APUP");
                } else if (!strcasecmp(optarg, "dn") 
                       || !strcasecmp(optarg, "down")) {
                    gpib_wrtf(gd, "APDN");
                } else {
                    if (amplstr(optarg, &a) < 0) {
                        fprintf(stderr, "%s: error parsing amplitude arg\n",
                                prog);
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
                    gpib_wrtf(gd, "AP%.2lfDM", a);
                }
                sleep(2); /* settling time (worst case) */
                break;
        }
    }

    gpib_fini(gd);

    exit(0);
}


/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
