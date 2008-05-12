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
#include <getopt.h>
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

char *prog = "";
static int verbose = 0;

#define OPTIONS "a:clviS"
static struct option longopts[] = {
    {"address",         required_argument, 0, 'a'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"get-idn",         no_argument,       0, 'i'},
    {"selftest",        no_argument,       0, 'S'},
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
"  -S,--selftest           perform instrument self-test\n"
           , prog, addr ? addr : "no default");
    exit(1);
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


/* Print instrument idn string.
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
    int selftest = 0;

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
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

    if (local)
        gpib_loc(gd); 

done:
    gpib_fini(gd);
    exit(exit_val);
}

/*
 * vi:tabstop=4 shiftwidth=4 expandtab
 */
