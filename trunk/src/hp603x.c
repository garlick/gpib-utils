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
#include <stdint.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <getopt.h>
#include <assert.h>
#include <sys/time.h>
#include <math.h>

#include "hp603x.h"
#include "gpib.h"
#include "util.h"

#define INSTRUMENT "hp603x"

#define MAXMODELBUF 512

char *prog = "";


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
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name                instrument address [%s]\n"
"  -c,--clear                    initialize instrument to default values\n"
"  -l,--local                    return instrument to local operation on exit\n"
"  -v,--verbose                  show protocol on stderr\n"
           , prog, addr ? addr : "no default");
    exit(1);
}

static int
_checkerror(gd_t gd)
{
    char tmpbuf[64];

    gpib_wrtf(gd, "%s", HP603X_ERR_QUERY);
    gpib_rdstr(gd, tmpbuf, sizeof(tmpbuf));

    return strtoul(tmpbuf, NULL, 10);
}

#if 0
static void
_reporterror(int err)
{
    /* presumably since error is a mask, we can have more than one? */
    if ((err & HP603X_ERROR_SYNTAX))
        fprintf(stderr, "%s: syntax error\n", prog);
    if ((err & HP603X_ERROR_EXEC))
        fprintf(stderr, "%s: execution error\n", prog);
    if ((err & HP603X_ERROR_TOOFAST))
        fprintf(stderr, "%s: trigger too fast\n", prog);
    if ((err & HP603X_ERROR_LOGIC))
        fprintf(stderr, "%s: logic error\n", prog);
    if ((err & HP603X_ERROR_POWER))
        fprintf(stderr, "%s: power supply failure\n", prog);
}
#endif

/* Interpet serial poll results (status byte)
 *   Return: 0=non-fatal/no error, >0=fatal, -1=retry.
 */
static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int res = 0;

    if ((status & HP603X_STATUS_PON)) {
        fprintf(stderr, "%s: srq: power-on SRQ occurred\n", prog);
        res = 1;
    }
    if ((status & HP603X_STATUS_FAU)) {
        fprintf(stderr, "%s: srq fault detected (see fault reg) \n", prog);
        /* FAULT? query */
        res = 1;
    }
    if ((status & HP603X_STATUS_ERR)) {
        fprintf(stderr, "%s: srq error detected (see err reg)\n", prog);
        /* ERR? query */
        //fprintf(stderr, "%s: error was %d\n", prog, _checkerror(gd));
        res = 1;
    }
    if ((status & HP603X_STATUS_RQS)) {
        fprintf(stderr, "%s: srq: rqs\n", prog);
        res = 1;
    }
#if 0
    if (res == 0 && !(status & HP603X_STATUS_RDY))
        res = -1;
#endif
    return res;
}

static void
hp603X_checkid(gd_t gd)
{
    char *supported[] = { 
        "Agilent 6030A", "HP 6030A",
        "Agilent 6031A", "HP 6031A",
        "Agilent 6032A", "HP 6032A",
        "Agilent 6033A", "HP 6033A",
        "Agilent 6035A", "HP 6035A",
        "Agilent 603XA", "HP 603XA", /* may have ", OPT xxx appended */
        NULL };
    char buf[MAXMODELBUF];
    int i, found = 0;

    gpib_wrtf(gd, "%s", HP603X_ID_QUERY);
    gpib_rdstr(gd, buf, sizeof(buf));

    for (i = 0; supported[i] != NULL; i++)
        if (!strncmp(buf, supported[i], strlen(supported[i])))
            found = 1;
    if (!found)
        fprintf(stderr, "%s: warning: model %s is unsupported\n", prog, buf);
}

int
main(int argc, char *argv[])
{
    char *addr = NULL;
    gd_t gd;
    int c;
    int verbose = 0;
    int clear = 0;
    int local = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'n': /* --name */
            addr = optarg;
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

    if (optind < argc)
        usage();

    if (!clear && !local)
        usage();

    if (!addr)
        addr = gpib_default_addr(INSTRUMENT);
    if (!addr) {
        fprintf(stderr, "%s: use --name to provide instrument address\n", prog);
        exit(1);
    }

    gd = gpib_init(addr, _interpret_status, 100000);
    if (!gd) {
        fprintf(stderr, "%s: device initialization failed for address %s\n", 
                prog, addr);
        exit(1);
    }
    gpib_set_verbose(gd, verbose);

    if (clear) {
        gpib_clr(gd, 2000000);
        gpib_wrtf(gd, "%s", HP603X_CLR); /* takes ~500ms */
        hp603X_checkid(gd);
    }

    gpib_wrtf(gd, "%s 5.0V", HP603X_VSET);

    if (local)
        gpib_loc(gd); 

    gpib_fini(gd);

    exit(0);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
