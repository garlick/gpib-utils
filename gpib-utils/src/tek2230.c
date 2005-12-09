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

/* My 2230 died, so this is just an aborted beginning.
 * I was able to dump the screen (at least the graticule) to thinkjet format
 * though.
 */

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

#define INSTRUMENT "tek2230" /* the /etc/gpib.conf entry */
#define BOARD       0        /* minor board number in /etc/gpib.conf */

static char *prog = "";
static int verbose = 0;

#define OPTIONS "n:clvpq"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"save-plot",       no_argument,       0, 'p'},
    {"query",           no_argument,       0, 'q'},
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
"  -p,--save-plot                save plot to stdout in HP ThinkJet format\n"
"  -q,--query                    query settings\n"
           , prog, INSTRUMENT);
    exit(1);
}

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int c, d;
    int clear = 0;
    int local = 0;
    int plot = 0;
    int query = 0;

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
        case 'p': /* --save-plot */
            plot = 1;
            break;
        case 'q': /* --query */
            query = 1;
            break;
        default:
            usage();
            break;
        }
    }

    if (!clear && !local && !plot && !query)
        usage();

    /* find device in /etc/gpib.conf */
    d = gpib_init(prog, instrument, verbose);

    /* clear instrument */
    if (clear)
        gpib_ibclr(d);

    if (query) {
        char tmpstr[1024];

        /* TESTING - just watch with -v for now */

        /* veritical */
        gpib_ibwrtf(d, "%s", "ch1?");
        gpib_ibrdstr(d, tmpstr, sizeof(tmpstr));
        gpib_ibwrtf(d, "%s", "ch2?");
        gpib_ibrdstr(d, tmpstr, sizeof(tmpstr));
        gpib_ibwrtf(d, "%s", "vmode?");
        gpib_ibrdstr(d, tmpstr, sizeof(tmpstr));
        gpib_ibwrtf(d, "%s", "probe?");
        gpib_ibrdstr(d, tmpstr, sizeof(tmpstr));

        /* horizontal */
        gpib_ibwrtf(d, "%s", "horizontal?");
        gpib_ibrdstr(d, tmpstr, sizeof(tmpstr));
    }

    /* save plot to stdout */
    if (plot) {
        uint8_t buf[1024*64]; /* 57809 bytes in tests so far */
        int count;

        gpib_ibwrtf(d, "%s", "plot speed:10,format:tjet,grat:on,auto:off,start");
        count = gpib_ibrd(d, buf, sizeof(buf));
        if (fwrite(buf, count, 1, stdout) != 1) {
            fprintf(stderr, "%s: error writing to stdout\n", prog);
            exit(1);
        }

        /* convert with Netpbm's thinkjettopbm(1) */
    }

    /* return front panel if requested */
    if (local)
        gpib_ibloc(d); 

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
