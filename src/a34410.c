/* This file is part of gpib-utils.
   For details, see http://sourceforge.net/projects/gpib-utils.

   Copyright (C) 2007 Jim Garlick <garlick@speakeasy.net>

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

/* Agilent 34410A DMM */

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
#include "util.h"
#include "a34410.h"

#define INSTRUMENT "a34410"

char *prog = "";
static int verbose = 0;

#define OPTIONS "n:clvisrpf:P:d"
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"get-idn",         no_argument,       0, 'i'},
    {0, 0, 0, 0},
};

static void 
usage(void)
{
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -a,--address            set instrument address [%s]\n"
"  -c,--clear              initialize instrument to default values\n"
"  -l,--local              return instrument to local operation on exit\n"
"  -v,--verbose            show protocol on stderr\n"
"  -i,--get-idn            return instrument idn string\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

    return err;
}


/* Print scope idn string.
 */
static int
_get_idn(gd_t gd)
{
    char tmpstr[64];
    int len;

    len = gpib_qry(gd, "*IDN?", tmpstr, sizeof(tmpstr) - 1);
    if (tmpstr[len - 1] == '\n')
        len -= 1;
    tmpstr[len] = '\0';
    fprintf(stderr, "%s: %s\n", prog,tmpstr);
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
        case 'i': /* --get-idn */
            get_idn = 1;
            todo++;
            break;
        default:
            usage();
            break;
        }
    }
    if (optind < argc || !todo)
        usage();

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
