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

/* HP 1650/1651A Logic Analyzer */

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

#include "libutil/util.h"
#include "gpib.h"

#define PATH_DATE "/bin/date"

#define INSTRUMENT "hp1650"

#define SETUP_STR_SIZE  3000
#define IMAGE_SIZE      6000000

/* Status byte values
 */
#define HP1650_STAT_OPER 128    /* an enabled condition in OPER has occurred */
#define HP1650_STAT_RQS   32    /* device is requesting service */
#define HP1650_STAT_ESB   16    /* an enabled condition in ESR has occurred */
#define HP1650_STAT_MAV    8    /* message(s) available in the output queue */
#define HP1650_STAT_MSG    4    /* advisory has been displayed on the scope */
#define HP1650_STAT_USR    2    /* an enabled user event has occurred */
#define HP1650_STAT_TRG    1    /* trigger has occurred */

/* Event status register values
 */
#define HP1650_ESR_PON   128    /* power on */
#define HP1650_ESR_URQ    64    /* user request */
#define HP1650_ESR_CME    32    /* command error */
#define HP1650_ESR_EXE    16    /* execution error */
#define HP1650_ESR_DDE     8    /* device dependent error */
#define HP1650_ESR_QYE     4    /* query error */
#define HP1650_ESR_RQL     2    /* request control */
#define HP1650_ESR_OPC     1    /* operation complete */

char *prog = "";
static char *options = OPTIONS_COMMON "lcisoRrSp";

#if HAVE_GETOPT_LONG
#define GETOPT(ac,av,opt,lopt) getopt_long(ac,av,opt,lopt,NULL)
static struct option longopts[] = {
    OPTIONS_COMMON_LONG,
    {"local",           no_argument,       0, 'l'},
    {"clear",           no_argument,       0, 'c'},
    {"get-idn",         no_argument,       0, 'i'},
    {"reset",           no_argument,       0, 'R'},
    {"selftest",        no_argument,       0, 'S'},
    {"print-screen",    no_argument,       0, 'p'},
    {0, 0, 0, 0},
};
#else
#define GETOPT(ac,av,opt,lopt) getopt(ac,av,opt)
#endif

static opt_desc_t optdesc[] = {
    OPTIONS_COMMON_DESC,
    {"l", "local",       "return instrument to front panel control" },
    {"c", "clear",       "initialize instrument to default values" },
    {"R", "reset",       "reset instrument to std. settings" },
    {"S", "selftest",    "execute instrument self test" },
    {"i", "get-idn",     "display instrument idn string" },
    {"p", "print-screen",  "send screen dump to stdout" },
    { 0, 0 },
};

int
main(int argc, char *argv[])
{
    gd_t gd;
    int c, exit_val = 0, print_usage = 0;

    gd = gpib_init_args(argc, argv, options, longopts, INSTRUMENT,
                        NULL, 0, &print_usage);
    if (print_usage)
        usage(optdesc);
    if (!gd)
        exit(1);

    /*gpib_set_eos(gd, '\n'); */
    gpib_set_reos(gd, 1); 

    optind = 0;
    while ((c = GETOPT(argc, argv, options, longopts)) != EOF) {
        switch (c) {
            OPTIONS_COMMON_SWITCH
            case 'f':	/* -f -P handled above */
            case 'P':
                break;
            case 'l': /* --local */
                gpib_loc(gd);
                break;
            case 'c': /* --clear */
                gpib_488_2_cls(gd);
                break;
            case 'R': /* --reset */
                gpib_488_2_rst(gd, 5);
                break;
            case 'S': /* --selftest */
                gpib_488_2_tst(gd, NULL);
                break;
            case 'i': /* --get-idn */
                gpib_488_2_idn(gd);
                break;
            case 'p': /* --print-screen */
                break;
        }
    }

    gpib_fini(gd);
    exit(exit_val);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
