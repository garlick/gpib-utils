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
#include <gpib/ib.h>

#include "dg535.h"
#include "units.h"

#define INSTRUMENT "dg535"  /* the /etc/gpib.conf entry */
#define BOARD       0       /* minor board number in /etc/gpib.conf */

static char *prog = "";
static int verbose = 0;

#define OPTIONS "n:clv"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
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
           , prog, INSTRUMENT);
    exit(1);
}


static void _ibclr(int d)
{
    ibclr(d);
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibclr timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibclr error %d\n", prog, iberr);
        exit(1);
    }
}

static void _ibwrtstr(int d, char *str)
{
    if (verbose)
        fprintf(stderr, "T: %s\n", str);
    ibwrt(d, str, strlen(str));
    if (ibsta & TIMO) {
        fprintf(stderr, "%s: ibwrt timeout\n", prog);
        exit(1);
    }
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibwrt error %d\n", prog, iberr);
        exit(1);
    }
    if (ibcnt != strlen(str)) {
        fprintf(stderr, "%s: ibwrt: short write\n", prog);
        exit(1);
    }
}

static void _ibwrtf(int d, char *fmt, ...)
{
        va_list ap;
        char *s;
        int n;

        va_start(ap, fmt);
        n = vasprintf(&s, fmt, ap);
        va_end(ap);
        if (n == -1) {
            fprintf(stderr, "%s: out of memory\n", prog);
            exit(1);
        }
        _ibwrtstr(d, s);
        free(s);
}

static void _ibloc(int d)
{
    ibloc(d);
    if (ibsta & ERR) {
        fprintf(stderr, "%s: ibloc error %d\n", prog, iberr);
        exit(1);
    }
}

int
main(int argc, char *argv[])
{
    char *instrument = INSTRUMENT;
    int c, d;
    int clear = 0;
    int local = 0;

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
        default:
            usage();
            break;
        }
    }

    if (!clear && !local)
        usage();

    /* find device in /etc/gpib.conf */
    d = ibfind(instrument);
    if (d < 0) {
        fprintf(stderr, "%s: not found: %s\n", prog, instrument);
        exit(1);
    }

    /* clear instrument to default settings */
    if (clear) {
        _ibclr(d);
        /*sleep(1);*/ /* instrument won't respond for about 1s after clear */
    }

    /* return front panel if requested */
    if (local)
        _ibloc(d); 

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
