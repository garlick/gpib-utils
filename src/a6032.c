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

/* Agilent 6000 series of oscilloscopes (DSO and MSO models) */

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
#include "a6032.h"

#define INSTRUMENT "a6032"

/* Constants from "SICL example in C", Agilent 6000 Series Programmer's Ref. */
#define SETUP_STR_SIZE  3000
#define IMAGE_SIZE      6000000

char *prog = "";
static int verbose = 0;

#define OPTIONS "n:clvisrpf:P:"
static struct option longopts[] = {
    {"name",            required_argument, 0, 'n'},
    {"clear",           no_argument,       0, 'c'},
    {"local",           no_argument,       0, 'l'},
    {"verbose",         no_argument,       0, 'v'},
    {"get-idn",         no_argument,       0, 'i'},
    {"save-setup",      no_argument,       0, 's'},
    {"restore-setup",   no_argument,       0, 'r'},
    {"print-screen",    no_argument,       0, 'p'},
    {"print-format",    required_argument, 0, 'f'},
    {"print-palette",   required_argument, 0, 'P'},
    {0, 0, 0, 0},
};

static void 
usage(void)
{
    char *addr = gpib_default_addr(INSTRUMENT);

    fprintf(stderr, 
"Usage: %s [--options]\n"
"  -n,--name name          instrument address [%s]\n"
"  -c,--clear              initialize instrument to default values\n"
"  -l,--local              return instrument to local operation on exit\n"
"  -v,--verbose            show protocol on stderr\n"
"  -i,--get-idn            return instrument idn string\n"
"  -s,--save-setup         save setup to stdout\n"
"  -r,--restore-setup      restore setup from stdin\n"
"  -p,--print-screen       send screen dump to stdout\n"
"  -f,--print-format       select print format (bmp|bmp8bit|png) [png]\n"
"  -P,--print-palette      select print palette (gray|col) [col]\n"
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
_check_esr(gd_t gd)
{
    int esr;
    int err = 0;

    gpib_wrtf(gd, "*ESR?");
    if (gpib_rdf(gd, "%d", &esr) != 1) {
        fprintf(stderr, "%s: error reading event status register\n", prog);
        return 0;
    }
    if (esr & A6032_ESR_PON) {
        fprintf(stderr, "%s: power on\n", prog);
    }
    if (esr & A6032_ESR_URQ) {
        fprintf(stderr, "%s: user request\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_CME) {
        fprintf(stderr, "%s: command error\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_EXE) {
        fprintf(stderr, "%s: execution error\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_DDE) {
        fprintf(stderr, "%s: device dependent error\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_QYE) {
        fprintf(stderr, "%s: query error\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_RQL) {
        fprintf(stderr, "%s: request control\n", prog);
        err = 1;
    }
    if (esr & A6032_ESR_OPC) {
        fprintf(stderr, "%s: operation complete \n", prog);
    }
    if (err) 
        _dump_error_queue(gd);
    return err;
}

static int
_interpret_status(gd_t gd, unsigned char status, char *msg)
{
    int err = 0;

    if (status & A6032_STAT_OPER) {
        /* check with :OPEREGISTER:CONDITION? */
    }
    if (status & A6032_STAT_RQS) {
        fprintf(stderr, "%s: device is requesting service\n", prog);
        /*err = 1;*/
    }
    if (status & A6032_STAT_ESB) {
        err = _check_esr(gd);
    }
    if (status & A6032_STAT_MAV) {
        fprintf(stderr, "%s: check output queue\n", prog);
    }
    if (status & A6032_STAT_MSG) {
        fprintf(stderr, "%s: advisory message has been displayed\n", prog);
    }
    if (status & A6032_STAT_USR) {
        fprintf(stderr, "%s: user event has occurred\n", prog);
    }
    if (status & A6032_STAT_TRG) {
        /*fprintf(stderr, "%s: trigger has occurred\n", prog);*/
    }
    /*gpib_wrtf(gd, "*CLS");*/
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

/* Save scope setup to stdout.
 */
static int
_save_setup(gd_t gd)
{
    unsigned char *buf = xmalloc(SETUP_STR_SIZE);
    int len;

    len = gpib_qry(gd, ":SYSTEM:SETUP?", buf, SETUP_STR_SIZE);
    if (gpib_decode_488_2_data(buf, &len, GPIB_VERIFY_DLAB) == -1) {
        fprintf(stderr, "%s: failed to decode 488.2 DLAB data\n", prog);
        return -1;
    }
    if (write_all(1, buf, len) < 0) {
        perror("write");
        return -1;
    }
    fprintf(stderr, "%s: save setup: %d bytes\n", prog, len);
    free(buf);
    return 0;
}


/* Restore scope setup from stdin.
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
    if (gpib_decode_488_2_data(buf, &len, GPIB_VERIFY_DLAB) == -1) {
        fprintf(stderr, "%s: failed to decode 488.2 DLAB data\n", prog);
        return -1;
    }
    gpib_wrtf(gd, ":SYSTEM:SETUP ");
    gpib_wrt(gd, buf, len);
    fprintf(stderr, "%s: restore setup: %d bytes\n", prog, len);
    return 0;
}

/* Print screen to stdout.
 */
static int
_print_screen(gd_t gd, char *fmt, char *palette)
{
    double tmout = gpib_get_timeout(gd);
    unsigned char *buf = xmalloc(IMAGE_SIZE);
    int len;
    char qry[64];

    snprintf(qry, sizeof(qry), ":DISPLAY:DATA? %s,scr,%s", fmt, palette);
    len = gpib_qry(gd, qry, buf, IMAGE_SIZE);

    if (gpib_decode_488_2_data(buf, &len, GPIB_DECODE_DLAB) == -1) {
        fprintf(stderr, "%s: failed to decode 488.2 data\n", prog);
        goto fail;
    }
    if (write_all(1, buf, len) < 0) {
        perror("write");
        goto fail;
    }
    fprintf(stderr, "%s: print screen: %d bytes (format=%s, palette=%s)\n", 
            prog, len, fmt, palette);
    free(buf);
    return 0;
fail:
    free(buf);
    return -1;
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
    int save_setup = 0;
    int restore_setup = 0;
    int print_screen = 0;
    char *print_format = "png";
    char *print_palette = "col";

    /*
     * Handle options.
     */
    prog = basename(argv[0]);
    while ((c = getopt_long(argc, argv, OPTIONS, longopts, NULL)) != EOF) {
        switch (c) {
        case 'n': /* --name */
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
        case 's': /* --save-setup */
            save_setup = 1;
            todo++;
            break;
        case 'r': /* --restore-setup */
            restore_setup = 1;
            todo++;
            break;
        case 'p': /* --print-screen */
            print_screen = 1;
            todo++;
            break;
        case 'f': /* --print-format */
            if (strncasecmp(optarg, "bmp", 3) && strncasecmp(optarg, "png", 3)
                    && strncasecmp(optarg, "bmp8bit", 7))
                usage();
            print_format = optarg;
            break;
        case 'P': /* --print-palette */
            if (strncasecmp(optarg, "col", 3) && strncasecmp(optarg, "gray", 4))
                usage();
            print_palette = optarg;
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
        fprintf(stderr, "%s: use --name to provide instrument address\n", prog);
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
        gpib_wrtf(gd, "*RST");
        sleep(1);
    }

    if (get_idn) {
        if (_get_idn(gd) == -1) {
            exit_val = 1;
            goto done;
        }
    }
    if (save_setup) {
        if (_save_setup(gd) == -1) {
            exit_val = 1;
            goto done;
        }
    }
    if (restore_setup) {
        if (_restore_setup(gd) == -1) {
            exit_val = 1;
            goto done;
        }
    }
    if (print_screen) {
        if (_print_screen(gd, print_format, print_palette) == -1) {
            exit_val = 1;
            goto done;
        }
    }

    /* :SYSTEM:DATE <date> */
    /* :SYSTEM:TIME <time> */
    /* :SYSTEM:DSP <string> */

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
